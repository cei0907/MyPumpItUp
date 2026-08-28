'use client';

import { ChangeEvent, MouseEvent, PointerEvent, useEffect, useMemo, useRef, useState } from 'react';
import {
  ChartDocument, ChartNote, HoldNote, Lane, beatToNumber, chartEndBeat, cloneChart, lanes, parsePdxChart, serializePdxChart,
} from './chart-format';

const sampleChart = `# Included State 22 sample. It uses the same .pdxchart format as the runtime.
schemaVersion=1
id=new-song-to-god-hold-playtest
delayMilliseconds=0

[tempo]
0=142

[notes]
hold=SW,13,17,8
hold=C,30,33,6
hold=SE,50,55,10
hold=NE,69,75,12
hold=SW,93,96,8
hold=NE,93,96,12
`;

type FileHandleLike = {
  getFile: () => Promise<File>;
  createWritable: () => Promise<{ write: (content: string) => Promise<void>; close: () => Promise<void> }>;
};
type EditorMode = 'select' | 'input' | 'erase';
type InputNoteType = 'tap' | 'hold';
type TimelinePosition = { lane: Lane; beat: number };
type DragState = { lane: Lane; startBeat: number; endBeat: number; resizeIndex: number | null };

const laneColors = { SW: '#4f9bff', NW: '#ff5b67', C: '#ffd34d', NE: '#ff5b67', SE: '#4f9bff' };
const laneGlyphs = { SW: '↙', NW: '↖', C: '✦', NE: '↗', SE: '↘' };
const errorText = (error: unknown) => error instanceof Error ? error.message : 'Unable to read the chart.';
const defaultTickCount = (startBeat: number, endBeat: number) => Math.max(1, Math.round((endBeat - startBeat) * 2));
const gridOptions = [
  { label: '1 beat', step: 1, denominator: 1 },
  { label: '1/2 beat', step: 1 / 2, denominator: 2 },
  { label: '1/3 beat · triplet', step: 1 / 3, denominator: 3 },
  { label: '1/4 beat · 16th', step: 1 / 4, denominator: 4 },
  { label: '1/6 beat · triplet 16th', step: 1 / 6, denominator: 6 },
  { label: '1/8 beat', step: 1 / 8, denominator: 8 },
  { label: '1/12 beat', step: 1 / 12, denominator: 12 },
  { label: '1/16 beat', step: 1 / 16, denominator: 16 },
];
const zoomOptions = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 2];
const greatestCommonDivisor = (left: number, right: number): number => right === 0 ? left : greatestCommonDivisor(right, left % right);
const formatSnappedBeat = (beat: number, denominator: number) => {
  const numerator = Math.round(beat * denominator);
  const divisor = greatestCommonDivisor(Math.abs(numerator), denominator);
  return denominator / divisor === 1 ? String(numerator / divisor) : `${numerator / divisor}/${denominator / divisor}`;
};

export default function Home() {
  const [chart, setChart] = useState<ChartDocument>(() => parsePdxChart(sampleChart));
  const [past, setPast] = useState<ChartDocument[]>([]);
  const [future, setFuture] = useState<ChartDocument[]>([]);
  const [selectedNote, setSelectedNote] = useState<number | null>(null);
  const [hoverPosition, setHoverPosition] = useState<TimelinePosition | null>(null);
  const [editorMode, setEditorMode] = useState<EditorMode>('select');
  const [inputNoteType, setInputNoteType] = useState<InputNoteType>('tap');
  const [gridOptionIndex, setGridOptionIndex] = useState(3);
  const [zoomIndex, setZoomIndex] = useState(3);
  const [requestedTimelineEndBeat, setRequestedTimelineEndBeat] = useState(64);
  const [viewStartBeat, setViewStartBeat] = useState(0);
  const [drag, setDrag] = useState<DragState | null>(null);
  const [notice, setNotice] = useState('Select mode is active. Click a note to inspect or edit it; empty cells do not create notes.');
  const [fileName, setFileName] = useState('new-song-to-god-hold-playtest.pdxchart');
  const [fileHandle, setFileHandle] = useState<FileHandleLike | null>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const chartRequiredEndBeat = useMemo(() => Math.ceil(chartEndBeat(chart) / 4) * 4 + 4, [chart]);
  const endBeat = Math.max(chartRequiredEndBeat, requestedTimelineEndBeat);
  const grid = gridOptions[gridOptionIndex];
  const visibleBeats = 16;
  const laneWidth = 72;
  const pixelsPerBeat = laneWidth / grid.step * zoomOptions[zoomIndex];
  const canvasWidth = lanes.length * laneWidth;
  const canvasHeight = visibleBeats * pixelsPerBeat + 44;
  const selected = selectedNote === null ? null : chart.notes[selectedNote] ?? null;
  const maximumViewStartBeat = Math.max(0, endBeat - visibleBeats);

  useEffect(() => {
    setViewStartBeat((current) => Math.min(current, maximumViewStartBeat));
  }, [maximumViewStartBeat]);

  const commit = (next: ChartDocument, message: string, nextSelection: number | null = null) => {
    setPast((entries) => [...entries.slice(-99), cloneChart(chart)]);
    setFuture([]); setChart(next); setSelectedNote(nextSelection); setNotice(message);
  };
  const replaceChart = (next: ChartDocument, message: string, name = fileName, handle: FileHandleLike | null = null) => {
    setPast([]); setFuture([]); setChart(next); setSelectedNote(null); setHoverPosition(null); setDrag(null); setFileName(name); setFileHandle(handle); setNotice(message);
  };
  const setActiveMode = (nextMode: EditorMode) => {
    setEditorMode(nextMode); setHoverPosition(null); setDrag(null);
    setNotice(nextMode === 'select'
      ? 'Select mode is active. Click a note to inspect or edit it; empty cells do not create notes.'
      : nextMode === 'erase'
        ? 'Delete mode is active. Click a note to remove it; empty cells do not change the chart.'
        : `${inputNoteType === 'tap' ? 'Tap' : 'Hold'} input is active. Click or drag only in an empty lane span.`);
  };
  const setActiveInputNoteType = (nextType: InputNoteType) => {
    setInputNoteType(nextType); setEditorMode('input'); setHoverPosition(null); setDrag(null);
    setNotice(nextType === 'tap' ? 'Tap input is active. Click an empty grid cell to add a tap.' : 'Hold input is active. Drag downward across one empty lane span.');
  };
  const changeZoom = (nextIndex: number) => {
    const boundedIndex = Math.max(0, Math.min(zoomOptions.length - 1, nextIndex));
    setZoomIndex(boundedIndex);
    setNotice(`Timeline zoom set to ${Math.round(zoomOptions[boundedIndex] * 100)}%.`);
  };
  const changeViewStart = (direction: number) => {
    const nextBeat = Math.max(0, Math.min(maximumViewStartBeat, viewStartBeat + direction * visibleBeats));
    setViewStartBeat(Math.floor(nextBeat / 4) * 4);
  };

  const noteBounds = (note: ChartNote) => {
    const startBeat = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
    const endBeatValue = note.type === 'tap' ? startBeat : beatToNumber(note.endBeat);
    const laneIndex = lanes.indexOf(note.lane);
    return {
      x: laneIndex * laneWidth + 6,
      width: laneWidth - 12,
      top: 44 + (startBeat - viewStartBeat) * pixelsPerBeat,
      height: note.type === 'hold' ? Math.max(laneWidth - 12, (endBeatValue - startBeat) * pixelsPerBeat) : laneWidth - 12,
    };
  };
  const timelinePosition = (event: PointerEvent<HTMLCanvasElement>): TimelinePosition | null => {
    const canvas = canvasRef.current; if (!canvas) return null;
    const bounds = canvas.getBoundingClientRect();
    const x = (event.clientX - bounds.left) * canvas.width / bounds.width;
    const y = (event.clientY - bounds.top) * canvas.height / bounds.height;
    if (y < 44) return null;
    const laneIndex = Math.floor(x / laneWidth);
    if (laneIndex < 0 || laneIndex >= lanes.length) return null;
    const rawBeat = Math.max(viewStartBeat, viewStartBeat + (y - 44) / pixelsPerBeat);
    return { lane: lanes[laneIndex], beat: Math.round(rawBeat / grid.step) * grid.step };
  };
  const hitNote = (position: TimelinePosition) => {
    const laneIndex = lanes.indexOf(position.lane);
    for (let index = chart.notes.length - 1; index >= 0; index -= 1) {
      const note = chart.notes[index]; if (lanes.indexOf(note.lane) !== laneIndex) continue;
      const start = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
      const end = note.type === 'tap' ? start : beatToNumber(note.endBeat);
      if (position.beat >= start && position.beat <= end) return index;
    }
    return null;
  };
  const collides = (candidate: HoldNote, ignoredIndex: number | null = null) => chart.notes.some((note, index) => {
    if (index === ignoredIndex || note.lane !== candidate.lane) return false;
    const start = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
    const end = note.type === 'tap' ? start : beatToNumber(note.endBeat);
    const candidateStart = beatToNumber(candidate.startBeat);
    const candidateEnd = beatToNumber(candidate.endBeat);
    return candidateStart <= end && candidateEnd >= start;
  });

  useEffect(() => {
    const canvas = canvasRef.current; const context = canvas?.getContext('2d'); if (!canvas || !context) return;
    context.clearRect(0, 0, canvas.width, canvas.height); context.fillStyle = '#101426'; context.fillRect(0, 0, canvas.width, canvas.height);
    lanes.forEach((lane, laneIndex) => {
      const x = laneIndex * laneWidth;
      context.fillStyle = laneIndex % 2 === 0 ? '#161b31' : '#13182b'; context.fillRect(x, 44, laneWidth, canvas.height - 44);
      context.fillStyle = laneColors[lane]; context.font = '700 26px Arial'; context.fillText(laneGlyphs[lane], x + 41, 29);
      context.strokeStyle = '#303854'; context.beginPath(); context.moveTo(x + laneWidth, 0); context.lineTo(x + laneWidth, canvas.height); context.stroke();
    });
    const firstLineIndex = Math.ceil(viewStartBeat / grid.step);
    const lastLineIndex = Math.floor((viewStartBeat + visibleBeats) / grid.step);
    for (let lineIndex = firstLineIndex; lineIndex <= lastLineIndex; lineIndex += 1) {
      const beat = lineIndex * grid.step;
      const y = 44 + (beat - viewStartBeat) * pixelsPerBeat;
      const wholeBeat = Math.abs(beat - Math.round(beat)) < 0.00001;
      const measure = wholeBeat && Math.round(beat) % 4 === 0;
      context.strokeStyle = measure ? '#596384' : wholeBeat ? '#303956' : '#242b46'; context.lineWidth = measure ? 2 : 1;
      context.beginPath(); context.moveTo(0, y); context.lineTo(canvas.width, y); context.stroke();
      if (measure) { context.fillStyle = '#9da9cd'; context.font = '600 11px Arial'; context.fillText(`M${Math.round(beat) / 4 + 1}`, 6, y + 14); }
    }
    if (editorMode === 'input' && hoverPosition !== null && drag === null && hitNote(hoverPosition) === null) {
      const laneIndex = lanes.indexOf(hoverPosition.lane);
      const top = 44 + (hoverPosition.beat - viewStartBeat) * pixelsPerBeat;
      const height = Math.max(5, grid.step * pixelsPerBeat);
      context.fillStyle = `${laneColors[hoverPosition.lane]}2f`;
      context.fillRect(laneIndex * laneWidth + 5, top, laneWidth - 10, height);
      context.strokeStyle = laneColors[hoverPosition.lane]; context.lineWidth = 2; context.setLineDash([4, 3]);
      context.strokeRect(laneIndex * laneWidth + 5, top, laneWidth - 10, height); context.setLineDash([]);
    }
    const drawNote = (note: ChartNote, index: number, preview = false) => {
      const { x, width, top, height } = noteBounds(note); const color = laneColors[note.lane];
      context.fillStyle = preview ? `${color}22` : `${color}33`; context.fillRect(x, top, width, height);
      context.strokeStyle = selectedNote === index ? '#ffffff' : color; context.lineWidth = selectedNote === index ? 3 : 2; context.strokeRect(x, top, width, height);
      const headHeight = Math.min(laneWidth - 12, height);
      context.fillStyle = color; context.fillRect(x, top, width, headHeight);
      context.fillStyle = '#101426'; context.font = `700 ${Math.max(16, Math.min(32, headHeight - 12))}px Arial`; context.fillText(laneGlyphs[note.lane], x + width / 2 - 12, top + headHeight / 2 + 11);
      if (note.type === 'hold' && height >= headHeight + 24) { context.fillStyle = '#dce5ff'; context.font = '600 11px Arial'; context.fillText(`${note.tickCount}`, x + width / 2 - 4, top + headHeight + 18); }
    };
    chart.notes.forEach((note, index) => {
      const startBeat = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
      const endBeatValue = note.type === 'tap' ? startBeat : beatToNumber(note.endBeat);
      if (endBeatValue >= viewStartBeat && startBeat <= viewStartBeat + visibleBeats) drawNote(note, index);
    });
    if (drag) {
      const preview: HoldNote = { type: 'hold', lane: drag.lane, startBeat: String(drag.startBeat), endBeat: String(drag.endBeat), tickCount: defaultTickCount(drag.startBeat, drag.endBeat) };
      drawNote(preview, -1, true);
    }
  }, [canvasHeight, canvasWidth, chart, drag, editorMode, endBeat, grid, hoverPosition, selectedNote, viewStartBeat]);

  const onPointerDown = (event: PointerEvent<HTMLCanvasElement>) => {
    const position = timelinePosition(event); if (!position) return;
    setHoverPosition(position);
    const hitIndex = hitNote(position);
    if (hitIndex !== null) {
      if (editorMode === 'erase') {
        commit({ ...chart, notes: chart.notes.filter((_, index) => index !== hitIndex) }, 'Deleted note.');
        return;
      }
      const hit = chart.notes[hitIndex]; setSelectedNote(hitIndex);
      if (editorMode === 'select' && hit.type === 'hold' && position.beat >= beatToNumber(hit.endBeat) - grid.step) {
        event.currentTarget.setPointerCapture(event.pointerId);
        setDrag({ lane: hit.lane, startBeat: beatToNumber(hit.startBeat), endBeat: beatToNumber(hit.endBeat), resizeIndex: hitIndex });
        setNotice('Drag the end of the selected hold to resize it.');
      } else setNotice(editorMode === 'select' ? `Selected ${hit.type} note on ${hit.lane}.` : 'This cell already has a note. Switch to Select mode to edit it.');
      return;
    }
    if (editorMode === 'select') { setSelectedNote(null); setNotice('No note at this position. Switch to Input to create a note.'); return; }
    if (editorMode === 'erase') { setSelectedNote(null); setNotice('No note at this position.'); return; }
    if (inputNoteType === 'tap') {
      const beat = formatSnappedBeat(position.beat, grid.denominator);
      commit({ ...chart, notes: [...chart.notes, { type: 'tap', lane: position.lane, beat }] }, `Added ${position.lane} tap at beat ${beat}.`);
      return;
    }
    event.currentTarget.setPointerCapture(event.pointerId);
    setDrag({ lane: position.lane, startBeat: position.beat, endBeat: position.beat + grid.step, resizeIndex: null });
    setNotice('Drag downward to choose the hold end.');
  };
  const onPointerMove = (event: PointerEvent<HTMLCanvasElement>) => {
    const position = timelinePosition(event);
    if (!drag) { setHoverPosition(editorMode === 'input' ? position : null); return; }
    if (!position || position.lane !== drag.lane) return;
    setDrag((current) => current ? { ...current, endBeat: Math.max(current.startBeat + grid.step, position.beat) } : null);
  };
  const onPointerUp = (event: PointerEvent<HTMLCanvasElement>) => {
    if (!drag) return;
    if (event.currentTarget.hasPointerCapture(event.pointerId)) event.currentTarget.releasePointerCapture(event.pointerId);
    const candidate: HoldNote = { type: 'hold', lane: drag.lane, startBeat: formatSnappedBeat(drag.startBeat, grid.denominator), endBeat: formatSnappedBeat(drag.endBeat, grid.denominator), tickCount: defaultTickCount(drag.startBeat, drag.endBeat) };
    const ignoredIndex = drag.resizeIndex;
    setDrag(null);
    setHoverPosition(editorMode === 'input' ? timelinePosition(event) : null);
    if (collides(candidate, ignoredIndex)) { setNotice('Hold overlaps another event in the same lane. Choose an empty range.'); return; }
    if (ignoredIndex !== null) {
      commit({ ...chart, notes: chart.notes.map((note, index) => index === ignoredIndex ? candidate : note) }, `Resized ${candidate.lane} hold to beat ${candidate.endBeat}.`);
    } else commit({ ...chart, notes: [...chart.notes, candidate] }, `Added ${candidate.lane} hold from beat ${candidate.startBeat} to ${candidate.endBeat}.`);
  };
  const onContextMenu = (event: MouseEvent<HTMLCanvasElement>) => {
    event.preventDefault();
    const position = timelinePosition(event as unknown as PointerEvent<HTMLCanvasElement>); if (!position) return;
    const hitIndex = hitNote(position);
    if (hitIndex === null) { setNotice('No note at this position.'); return; }
    commit({ ...chart, notes: chart.notes.filter((_, index) => index !== hitIndex) }, 'Deleted note with the secondary click.');
  };

  const undo = () => { const previous = past.at(-1); if (!previous) { setNotice('Nothing to undo.'); return; } setPast((entries) => entries.slice(0, -1)); setFuture((entries) => [cloneChart(chart), ...entries]); setChart(previous); setSelectedNote(null); setNotice('Undid the last edit.'); };
  const redo = () => { const next = future[0]; if (!next) { setNotice('Nothing to redo.'); return; } setFuture((entries) => entries.slice(1)); setPast((entries) => [...entries, cloneChart(chart)]); setChart(next); setSelectedNote(null); setNotice('Redid the edit.'); };
  const removeSelected = () => { if (selectedNote === null) { setNotice('Select a note before removing it.'); return; } commit({ ...chart, notes: chart.notes.filter((_, index) => index !== selectedNote) }, 'Selected note removed.'); };
  const updateSelectedNote = (field: 'lane' | 'beat' | 'startBeat' | 'endBeat' | 'tickCount', value: string) => {
    if (selectedNote === null || selected === null) return;
    const changed = { ...selected, [field]: field === 'tickCount' ? Math.max(1, Number(value) || 1) : value } as ChartNote;
    const startBeat = beatToNumber(changed.type === 'tap' ? changed.beat : changed.startBeat);
    const endBeat = changed.type === 'tap' ? startBeat : beatToNumber(changed.endBeat);
    if (!Number.isFinite(startBeat) || !Number.isFinite(endBeat) || startBeat < 0) { setNotice('Beat must be zero or later and use a number or fraction.'); return; }
    if (changed.type === 'hold' && endBeat <= startBeat) { setNotice('A hold end must be after its start.'); return; }
    if (changed.type === 'hold' && collides(changed, selectedNote)) { setNotice('Hold overlaps another event in the same lane.'); return; }
    if (changed.type === 'tap') {
      if (chart.notes.some((note, index) => index !== selectedNote && note.type === 'hold' && note.lane === changed.lane
        && beatToNumber(note.startBeat) <= startBeat && startBeat <= beatToNumber(note.endBeat))) { setNotice('Tap overlaps a hold in the same lane.'); return; }
    }
    commit({ ...chart, notes: chart.notes.map((note, index) => index === selectedNote ? changed : note) }, 'Updated selected note.', selectedNote);
  };
  const loadText = (text: string, name: string, handle: FileHandleLike | null = null) => { try { replaceChart(parsePdxChart(text), `Loaded ${name}.`, name, handle); } catch (error) { setNotice(errorText(error)); } };
  const openChart = async () => {
    const picker = (window as typeof window & { showOpenFilePicker?: () => Promise<FileHandleLike[]> }).showOpenFilePicker;
    if (!picker) { inputRef.current?.click(); return; }
    try { const [handle] = await picker(); const file = await handle.getFile(); loadText(await file.text(), file.name, handle); } catch (error) { if (!(error instanceof DOMException && error.name === 'AbortError')) setNotice(errorText(error)); }
  };
  const onInputFile = async (event: ChangeEvent<HTMLInputElement>) => { const file = event.target.files?.[0]; if (file) loadText(await file.text(), file.name); event.target.value = ''; };
  const saveChart = async () => {
    try {
      const content = serializePdxChart(chart);
      if (fileHandle) { const writable = await fileHandle.createWritable(); await writable.write(content); await writable.close(); setNotice(`Saved ${fileName} in place.`); return; }
      const url = URL.createObjectURL(new Blob([content], { type: 'text/plain;charset=utf-8' })); const anchor = document.createElement('a'); anchor.href = url; anchor.download = fileName.endsWith('.pdxchart') ? fileName : `${fileName}.pdxchart`; anchor.click(); URL.revokeObjectURL(url); setNotice('Downloaded the compatible .pdxchart file. Edge can save directly into a selected file.');
    } catch (error) { setNotice(errorText(error)); }
  };
  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.target instanceof HTMLInputElement || event.target instanceof HTMLSelectElement || event.target instanceof HTMLTextAreaElement) return;
      const key = event.key.toLowerCase();
      if ((event.ctrlKey || event.metaKey) && key === 'z') { event.preventDefault(); undo(); return; }
      if ((event.ctrlKey || event.metaKey) && key === 'y') { event.preventDefault(); redo(); return; }
      if (event.key === 'Delete' || event.key === 'Backspace') { event.preventDefault(); removeSelected(); return; }
      if (key === 'v') { setActiveMode('select'); return; }
      if (key === 'i') { setActiveMode('input'); return; }
      if (key === 'x') { setActiveMode('erase'); return; }
      if (event.key === '1') { setActiveInputNoteType('tap'); return; }
      if (event.key === '2') setActiveInputNoteType('hold');
    };
    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, [past, future, selectedNote, chart, inputNoteType]);

  return <main className="editor-shell">
    <header className="topbar"><div><p className="eyebrow">PUMPDX / STAGE 04</p><h1>Chart Editor</h1></div><div className="topbar-actions"><button className="quiet-button" onClick={() => loadText(sampleChart, 'new-song-to-god-hold-playtest.pdxchart')}>Load hold demo</button><button className="quiet-button" onClick={openChart}>Open .pdxchart</button><button className="primary-button" onClick={saveChart}>Save chart</button><input ref={inputRef} className="hidden-input" type="file" accept=".pdxchart,text/plain" onChange={onInputFile} /></div></header>
    <section className="editor-grid"><aside className="inspector">
      <section className="panel"><p className="panel-kicker">Chart metadata</p><label>Chart ID<input value={chart.id} onChange={(event) => setChart((current) => ({ ...current, id: event.target.value }))} /></label><label>Initial BPM<input type="number" min="1" value={chart.tempo[0]?.bpm ?? 120} onChange={(event) => { const bpm = Number(event.target.value); if (bpm > 0) setChart((current) => ({ ...current, tempo: [{ ...current.tempo[0], bpm }] })); }} /></label><label>Delay (ms)<input type="number" min="0" step="1" value={chart.delayMilliseconds} onChange={(event) => { const delayMilliseconds = Number(event.target.value); if (Number.isInteger(delayMilliseconds) && delayMilliseconds >= 0) setChart((current) => ({ ...current, delayMilliseconds })); }} /></label><p className="hint">Delay waits after audio starts, but M1 always remains Beat 0. Place the first note directly on M1’s desired beat.</p></section>
      <section className="panel"><p className="panel-kicker">Editor mode</p><div className="tool-row mode-row"><button className={editorMode === 'select' ? 'tool-button active' : 'tool-button'} onClick={() => setActiveMode('select')}>Select</button><button className={editorMode === 'input' ? 'tool-button active' : 'tool-button'} onClick={() => setActiveMode('input')}>Input</button><button className={editorMode === 'erase' ? 'tool-button danger-active' : 'tool-button'} onClick={() => setActiveMode('erase')}>Delete</button></div><p className="hint">Select never adds a note. Input creates notes only in empty cells. Delete removes the clicked note.</p></section>
      <section className="panel"><p className="panel-kicker">Input settings</p><label>Note type<select value={inputNoteType} onChange={(event) => setActiveInputNoteType(event.target.value as InputNoteType)}><option value="tap">Tap note</option><option value="hold">Long note</option></select></label><label>Grid snap<select value={gridOptionIndex} onChange={(event) => { const index = Number(event.target.value); setGridOptionIndex(index); setNotice(`Grid snap changed to ${gridOptions[index].label}.`); }}>{gridOptions.map((option, index) => <option key={option.label} value={index}>{option.label}</option>)}</select></label><p className="hint">Choose the grid first. 1/3 and 1/6 keep triplet beats as exact fractions.</p></section>
      <section className="panel"><p className="panel-kicker">Timeline range</p><label>End beat<input type="number" min={chartRequiredEndBeat} step="4" value={endBeat} onChange={(event) => { const value = Number(event.target.value); if (Number.isFinite(value)) setRequestedTimelineEndBeat(Math.max(chartRequiredEndBeat, Math.ceil(value / 4) * 4)); }} /></label><label>View starts at beat<input type="number" min="0" max={maximumViewStartBeat} step="4" value={viewStartBeat} onChange={(event) => { const value = Number(event.target.value); if (Number.isFinite(value)) setViewStartBeat(Math.max(0, Math.min(maximumViewStartBeat, Math.floor(value / 4) * 4))); }} /></label><button className="quiet-button full-button" onClick={() => { setRequestedTimelineEndBeat(endBeat + 16); setNotice(`Timeline extended to beat ${endBeat + 16}.`); }}>Add 16 beats</button><p className="hint">Each view shows four measures with square grid cells. Move the view to edit later measures safely.</p></section>
      {selected && <section className="panel"><p className="panel-kicker">Selected {selected.type} note</p><label>Panel<select value={selected.lane} onChange={(event) => updateSelectedNote('lane', event.target.value)}>{lanes.map((lane) => <option key={lane} value={lane}>{lane}</option>)}</select></label>{selected.type === 'tap' ? <label>Beat<input value={selected.beat} onChange={(event) => updateSelectedNote('beat', event.target.value)} /></label> : <><label>Start beat<input value={selected.startBeat} onChange={(event) => updateSelectedNote('startBeat', event.target.value)} /></label><label>End beat<input value={selected.endBeat} onChange={(event) => updateSelectedNote('endBeat', event.target.value)} /></label><label>Tick count<input type="number" min="1" value={selected.tickCount} onChange={(event) => updateSelectedNote('tickCount', event.target.value)} /></label></>}<p className="hint">Panel is a fixed choice; beats accept integers, fractions, and mixed fractions.</p></section>}
      <section className="panel"><p className="panel-kicker">Edit history</p><div className="button-row"><button className="quiet-button" onClick={undo} disabled={past.length === 0}>Undo</button><button className="quiet-button" onClick={redo} disabled={future.length === 0}>Redo</button></div><button className="danger-button" onClick={removeSelected}>Remove selected note</button></section>
      <section className="panel status-panel" aria-live="polite"><p className="panel-kicker">Editor status</p><p>{notice}</p></section>
    </aside><section className="workspace"><div className="workspace-heading"><div><p className="panel-kicker">5-panel vertical timeline · {grid.label} snap</p><h2>{fileName}</h2></div><div className="workspace-actions"><div className="zoom-control" aria-label="Timeline view"><button className="quiet-button compact-button" onClick={() => changeViewStart(-1)} disabled={viewStartBeat === 0} aria-label="Previous four measures">‹</button><span>M{viewStartBeat / 4 + 1}–M{(viewStartBeat + visibleBeats) / 4}</span><button className="quiet-button compact-button" onClick={() => changeViewStart(1)} disabled={viewStartBeat >= maximumViewStartBeat} aria-label="Next four measures">›</button></div><div className="zoom-control" aria-label="Timeline zoom"><span>Zoom</span><button className="quiet-button compact-button" onClick={() => changeZoom(zoomIndex - 1)} disabled={zoomIndex === 0} aria-label="Zoom out">−</button><select value={zoomIndex} onChange={(event) => changeZoom(Number(event.target.value))} aria-label="Timeline zoom percentage">{zoomOptions.map((zoom, index) => <option key={zoom} value={index}>{Math.round(zoom * 100)}%</option>)}</select><button className="quiet-button compact-button" onClick={() => changeZoom(zoomIndex + 1)} disabled={zoomIndex === zoomOptions.length - 1} aria-label="Zoom in">+</button></div><span>{chart.notes.length} events · ends at beat {endBeat}</span></div></div><div className="timeline-frame"><canvas className={editorMode === 'select' ? 'select-cursor' : editorMode === 'erase' ? 'erase-cursor' : 'input-cursor'} ref={canvasRef} width={canvasWidth} height={canvasHeight} aria-label="Five lane vertical chart timeline. Select mode inspects notes, Input mode creates notes, and Delete mode removes notes." onPointerDown={onPointerDown} onPointerMove={onPointerMove} onPointerUp={onPointerUp} onPointerLeave={() => setHoverPosition(null)} onContextMenu={onContextMenu} /></div><div className="timeline-footer"><span>{editorMode === 'select' ? 'Select: click a note to inspect it. Empty cells are safe.' : editorMode === 'erase' ? 'Delete: click a note to remove it. Empty cells are safe.' : inputNoteType === 'tap' ? 'Input: click an empty lane cell to add a tap note.' : 'Input: drag downward in an empty lane span to add a long note.'}</span><span>Square cells show the pattern clearly · Zoom: − / + · Shortcuts: V Select · I Input · X Delete · 1 Tap · 2 Hold · Del Remove · Ctrl+Z/Y Undo/Redo</span></div></section></section>
  </main>;
}
