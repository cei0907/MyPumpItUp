'use client';

import { ChangeEvent, PointerEvent, useEffect, useMemo, useRef, useState } from 'react';
import {
  ChartDocument, ChartNote, HoldNote, Lane, beatToNumber, chartEndBeat, cloneChart, lanes, parsePdxChart, serializePdxChart,
} from './chart-format';

const sampleChart = `# Included State 22 sample. It uses the same .pdxchart format as the runtime.
schemaVersion=1
id=new-song-to-god-hold-playtest

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
type Tool = 'tap' | 'hold';
type TimelinePosition = { lane: Lane; beat: number };
type DragState = { lane: Lane; startBeat: number; endBeat: number; resizeIndex: number | null };

const laneColors = { SW: '#ff5c83', NW: '#ffb65f', C: '#8f7cff', NE: '#58d9c3', SE: '#5c9dff' };
const errorText = (error: unknown) => error instanceof Error ? error.message : 'Unable to read the chart.';
const defaultTickCount = (startBeat: number, endBeat: number) => Math.max(1, Math.round((endBeat - startBeat) * 2));

export default function Home() {
  const [chart, setChart] = useState<ChartDocument>(() => parsePdxChart(sampleChart));
  const [past, setPast] = useState<ChartDocument[]>([]);
  const [future, setFuture] = useState<ChartDocument[]>([]);
  const [selectedNote, setSelectedNote] = useState<number | null>(null);
  const [tool, setTool] = useState<Tool>('tap');
  const [drag, setDrag] = useState<DragState | null>(null);
  const [notice, setNotice] = useState('Tap tool selected. Click an empty lane cell to add a tap note.');
  const [fileName, setFileName] = useState('new-song-to-god-hold-playtest.pdxchart');
  const [fileHandle, setFileHandle] = useState<FileHandleLike | null>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const endBeat = useMemo(() => Math.ceil(chartEndBeat(chart) / 4) * 4 + 4, [chart]);
  const pixelsPerBeat = 54;
  const laneWidth = 112;
  const canvasWidth = lanes.length * laneWidth;
  const canvasHeight = Math.max(720, endBeat * pixelsPerBeat + 44);
  const selected = selectedNote === null ? null : chart.notes[selectedNote] ?? null;

  const commit = (next: ChartDocument, message: string) => {
    setPast((entries) => [...entries.slice(-99), cloneChart(chart)]);
    setFuture([]); setChart(next); setSelectedNote(null); setNotice(message);
  };
  const replaceChart = (next: ChartDocument, message: string, name = fileName, handle: FileHandleLike | null = null) => {
    setPast([]); setFuture([]); setChart(next); setSelectedNote(null); setDrag(null); setFileName(name); setFileHandle(handle); setNotice(message);
  };
  const setActiveTool = (nextTool: Tool) => {
    setTool(nextTool); setDrag(null); setNotice(nextTool === 'tap' ? 'Tap tool selected.' : 'Hold tool selected. Drag across one lane to create a hold.');
  };

  const noteBounds = (note: ChartNote) => {
    const startBeat = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
    const endBeatValue = note.type === 'tap' ? startBeat : beatToNumber(note.endBeat);
    const laneIndex = lanes.indexOf(note.lane);
    return {
      x: laneIndex * laneWidth + 24,
      width: 64,
      top: 44 + startBeat * pixelsPerBeat,
      height: note.type === 'hold' ? Math.max(28, (endBeatValue - startBeat) * pixelsPerBeat) : 26,
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
    return { lane: lanes[laneIndex], beat: Math.max(0, Math.round((y - 44) / pixelsPerBeat)) };
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
      context.fillStyle = laneColors[lane]; context.font = '700 16px Arial'; context.fillText(lane, x + 40, 28);
      context.strokeStyle = '#303854'; context.beginPath(); context.moveTo(x + laneWidth, 0); context.lineTo(x + laneWidth, canvas.height); context.stroke();
    });
    for (let beat = 0; beat <= endBeat; beat += 1) {
      const y = 44 + beat * pixelsPerBeat;
      context.strokeStyle = beat % 4 === 0 ? '#596384' : '#242b46'; context.lineWidth = beat % 4 === 0 ? 2 : 1;
      context.beginPath(); context.moveTo(0, y); context.lineTo(canvas.width, y); context.stroke();
      if (beat % 4 === 0) { context.fillStyle = '#9da9cd'; context.font = '600 11px Arial'; context.fillText(`M${beat / 4 + 1}`, 6, y + 14); }
    }
    const drawNote = (note: ChartNote, index: number, preview = false) => {
      const { x, width, top, height } = noteBounds(note); const color = laneColors[note.lane];
      context.fillStyle = preview ? `${color}22` : `${color}33`; context.fillRect(x, top, width, height);
      context.strokeStyle = selectedNote === index ? '#ffffff' : color; context.lineWidth = selectedNote === index ? 3 : 2; context.strokeRect(x, top, width, height);
      context.fillStyle = color; context.fillRect(x, top, width, Math.min(26, height));
      if (note.type === 'hold') { context.fillStyle = '#dce5ff'; context.font = '600 11px Arial'; context.fillText(`${note.tickCount}`, x + 26, top + 47); }
    };
    chart.notes.forEach((note, index) => drawNote(note, index));
    if (drag) {
      const preview: HoldNote = { type: 'hold', lane: drag.lane, startBeat: String(drag.startBeat), endBeat: String(drag.endBeat), tickCount: defaultTickCount(drag.startBeat, drag.endBeat) };
      drawNote(preview, -1, true);
    }
  }, [canvasHeight, canvasWidth, chart, drag, endBeat, selectedNote]);

  const onPointerDown = (event: PointerEvent<HTMLCanvasElement>) => {
    const position = timelinePosition(event); if (!position) return;
    const hitIndex = hitNote(position);
    if (hitIndex !== null) {
      const hit = chart.notes[hitIndex]; setSelectedNote(hitIndex);
      if (tool === 'hold' && hit.type === 'hold' && position.beat >= beatToNumber(hit.endBeat) - 1) {
        event.currentTarget.setPointerCapture(event.pointerId);
        setDrag({ lane: hit.lane, startBeat: beatToNumber(hit.startBeat), endBeat: beatToNumber(hit.endBeat), resizeIndex: hitIndex });
        setNotice('Drag the end of the selected hold to resize it.');
      } else setNotice(`Selected ${hit.type} note on ${hit.lane}.`);
      return;
    }
    if (tool === 'tap') {
      commit({ ...chart, notes: [...chart.notes, { type: 'tap', lane: position.lane, beat: String(position.beat) }] }, `Added ${position.lane} tap at beat ${position.beat}.`);
      return;
    }
    event.currentTarget.setPointerCapture(event.pointerId);
    setDrag({ lane: position.lane, startBeat: position.beat, endBeat: position.beat + 1, resizeIndex: null });
    setNotice('Drag right to choose the hold end.');
  };
  const onPointerMove = (event: PointerEvent<HTMLCanvasElement>) => {
    if (!drag) return;
    const position = timelinePosition(event); if (!position || position.lane !== drag.lane) return;
    setDrag((current) => current ? { ...current, endBeat: Math.max(current.startBeat + 1, position.beat) } : null);
  };
  const onPointerUp = (event: PointerEvent<HTMLCanvasElement>) => {
    if (!drag) return;
    if (event.currentTarget.hasPointerCapture(event.pointerId)) event.currentTarget.releasePointerCapture(event.pointerId);
    const candidate: HoldNote = { type: 'hold', lane: drag.lane, startBeat: String(drag.startBeat), endBeat: String(drag.endBeat), tickCount: defaultTickCount(drag.startBeat, drag.endBeat) };
    const ignoredIndex = drag.resizeIndex;
    setDrag(null);
    if (collides(candidate, ignoredIndex)) { setNotice('Hold overlaps another event in the same lane. Choose an empty range.'); return; }
    if (ignoredIndex !== null) {
      commit({ ...chart, notes: chart.notes.map((note, index) => index === ignoredIndex ? candidate : note) }, `Resized ${candidate.lane} hold to beat ${candidate.endBeat}.`);
    } else commit({ ...chart, notes: [...chart.notes, candidate] }, `Added ${candidate.lane} hold from beat ${candidate.startBeat} to ${candidate.endBeat}.`);
  };

  const undo = () => { const previous = past.at(-1); if (!previous) { setNotice('Nothing to undo.'); return; } setPast((entries) => entries.slice(0, -1)); setFuture((entries) => [cloneChart(chart), ...entries]); setChart(previous); setSelectedNote(null); setNotice('Undid the last edit.'); };
  const redo = () => { const next = future[0]; if (!next) { setNotice('Nothing to redo.'); return; } setFuture((entries) => entries.slice(1)); setPast((entries) => [...entries, cloneChart(chart)]); setChart(next); setSelectedNote(null); setNotice('Redid the edit.'); };
  const removeSelected = () => { if (selectedNote === null) { setNotice('Select a note before removing it.'); return; } commit({ ...chart, notes: chart.notes.filter((_, index) => index !== selectedNote) }, 'Selected note removed.'); };
  const updateSelectedHold = (field: 'startBeat' | 'endBeat' | 'tickCount', value: string) => {
    if (selectedNote === null || selected?.type !== 'hold') return;
    const changed = { ...selected, [field]: field === 'tickCount' ? Math.max(1, Number(value) || 1) : value } as HoldNote;
    if (beatToNumber(changed.endBeat) <= beatToNumber(changed.startBeat)) { setNotice('A hold end must be after its start.'); return; }
    if (collides(changed, selectedNote)) { setNotice('Hold overlaps another event in the same lane.'); return; }
    commit({ ...chart, notes: chart.notes.map((note, index) => index === selectedNote ? changed : note) }, 'Updated selected hold.');
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

  return <main className="editor-shell">
    <header className="topbar"><div><p className="eyebrow">PUMPDX / STAGE 04</p><h1>Chart Editor</h1></div><div className="topbar-actions"><button className="quiet-button" onClick={() => loadText(sampleChart, 'new-song-to-god-hold-playtest.pdxchart')}>Load sample</button><button className="quiet-button" onClick={openChart}>Open .pdxchart</button><button className="primary-button" onClick={saveChart}>Save chart</button><input ref={inputRef} className="hidden-input" type="file" accept=".pdxchart,text/plain" onChange={onInputFile} /></div></header>
    <section className="editor-grid"><aside className="inspector">
      <section className="panel"><p className="panel-kicker">Chart metadata</p><label>Chart ID<input value={chart.id} onChange={(event) => setChart((current) => ({ ...current, id: event.target.value }))} /></label><label>Initial BPM<input type="number" min="1" value={chart.tempo[0]?.bpm ?? 120} onChange={(event) => { const bpm = Number(event.target.value); if (bpm > 0) setChart((current) => ({ ...current, tempo: [{ ...current.tempo[0], bpm }] })); }} /></label><p className="hint">State 22 keeps one initial BPM. Tempo changes arrive with the advanced timing state.</p></section>
      <section className="panel"><p className="panel-kicker">Note tool</p><div className="tool-row"><button className={tool === 'tap' ? 'tool-button active' : 'tool-button'} onClick={() => setActiveTool('tap')}>Tap</button><button className={tool === 'hold' ? 'tool-button active' : 'tool-button'} onClick={() => setActiveTool('hold')}>Hold</button></div><p className="hint">Hold: drag downward in one lane. Drag a selected hold’s lower tail to resize it.</p></section>
      {selected?.type === 'hold' && <section className="panel"><p className="panel-kicker">Selected hold · {selected.lane}</p><label>Start beat<input value={selected.startBeat} onChange={(event) => updateSelectedHold('startBeat', event.target.value)} /></label><label>End beat<input value={selected.endBeat} onChange={(event) => updateSelectedHold('endBeat', event.target.value)} /></label><label>Tick count<input type="number" min="1" value={selected.tickCount} onChange={(event) => updateSelectedHold('tickCount', event.target.value)} /></label></section>}
      <section className="panel"><p className="panel-kicker">Edit history</p><div className="button-row"><button className="quiet-button" onClick={undo} disabled={past.length === 0}>Undo</button><button className="quiet-button" onClick={redo} disabled={future.length === 0}>Redo</button></div><button className="danger-button" onClick={removeSelected}>Remove selected note</button></section>
      <section className="panel status-panel" aria-live="polite"><p className="panel-kicker">Editor status</p><p>{notice}</p></section>
    </aside><section className="workspace"><div className="workspace-heading"><div><p className="panel-kicker">5-panel vertical timeline · 1 beat snap</p><h2>{fileName}</h2></div><span>{chart.notes.length} events</span></div><div className="timeline-frame"><canvas ref={canvasRef} width={canvasWidth} height={canvasHeight} aria-label="Five lane vertical chart timeline. Use the selected tool to create or edit notes." onPointerDown={onPointerDown} onPointerMove={onPointerMove} onPointerUp={onPointerUp} /></div><div className="timeline-footer"><span>{tool === 'tap' ? 'Click an empty lane cell to add a tap note.' : 'Drag downward in an empty lane span to add a long note.'}</span><span>Holds use 2 ticks per beat by default; adjust the selected hold when needed.</span></div></section></section>
  </main>;
}
