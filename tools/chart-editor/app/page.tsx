'use client';

import { ChangeEvent, useEffect, useMemo, useRef, useState } from 'react';
import { ChartDocument, beatToNumber, chartEndBeat, cloneChart, lanes, parsePdxChart, serializePdxChart } from './chart-format';

const sampleChart = `# Included State 21 sample. It uses the same .pdxchart format as the runtime.
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

const laneColors = { SW: '#ff5c83', NW: '#ffb65f', C: '#8f7cff', NE: '#58d9c3', SE: '#5c9dff' };
const errorText = (error: unknown) => error instanceof Error ? error.message : 'Unable to read the chart.';

export default function Home() {
  const [chart, setChart] = useState<ChartDocument>(() => parsePdxChart(sampleChart));
  const [past, setPast] = useState<ChartDocument[]>([]);
  const [future, setFuture] = useState<ChartDocument[]>([]);
  const [selectedNote, setSelectedNote] = useState<number | null>(null);
  const [notice, setNotice] = useState('Included sample chart loaded. Click an empty lane cell to add a tap note.');
  const [fileName, setFileName] = useState('new-song-to-god-hold-playtest.pdxchart');
  const [fileHandle, setFileHandle] = useState<FileHandleLike | null>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const endBeat = useMemo(() => Math.ceil(chartEndBeat(chart) / 4) * 4 + 4, [chart]);
  const pixelsPerBeat = 54;
  const laneHeight = 72;
  const canvasWidth = Math.max(1080, endBeat * pixelsPerBeat + 116);
  const canvasHeight = lanes.length * laneHeight + 38;

  const commit = (next: ChartDocument, message: string) => {
    setPast((entries) => [...entries.slice(-99), cloneChart(chart)]);
    setFuture([]);
    setChart(next);
    setSelectedNote(null);
    setNotice(message);
  };

  const replaceChart = (next: ChartDocument, message: string, name = fileName, handle: FileHandleLike | null = null) => {
    setPast([]); setFuture([]); setChart(next); setSelectedNote(null); setFileName(name); setFileHandle(handle); setNotice(message);
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    const context = canvas?.getContext('2d');
    if (!canvas || !context) return;
    context.clearRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = '#101426'; context.fillRect(0, 0, canvas.width, canvas.height);
    for (let beat = 0; beat <= endBeat; beat += 1) {
      const x = 116 + beat * pixelsPerBeat;
      context.strokeStyle = beat % 4 === 0 ? '#596384' : '#242b46'; context.lineWidth = beat % 4 === 0 ? 2 : 1;
      context.beginPath(); context.moveTo(x, 0); context.lineTo(x, canvas.height); context.stroke();
      if (beat % 4 === 0) { context.fillStyle = '#9da9cd'; context.font = '600 12px Arial'; context.fillText(`M${beat / 4 + 1}`, x + 8, 20); }
    }
    lanes.forEach((lane, laneIndex) => {
      const top = 38 + laneIndex * laneHeight;
      context.fillStyle = laneIndex % 2 === 0 ? '#161b31' : '#13182b'; context.fillRect(0, top, canvas.width, laneHeight);
      context.fillStyle = laneColors[lane]; context.font = '700 16px Arial'; context.fillText(lane, 42, top + 43);
      context.strokeStyle = '#303854'; context.beginPath(); context.moveTo(0, top + laneHeight); context.lineTo(canvas.width, top + laneHeight); context.stroke();
    });
    chart.notes.forEach((note, index) => {
      const laneIndex = lanes.indexOf(note.lane);
      const top = 38 + laneIndex * laneHeight + 17;
      const startBeat = beatToNumber(note.type === 'tap' ? note.beat : note.startBeat);
      const endBeatValue = note.type === 'tap' ? startBeat : beatToNumber(note.endBeat);
      const x = 116 + startBeat * pixelsPerBeat;
      const width = note.type === 'hold' ? Math.max(28, (endBeatValue - startBeat) * pixelsPerBeat) : 26;
      const color = laneColors[note.lane];
      context.fillStyle = `${color}33`; context.fillRect(x, top, width, 38);
      context.strokeStyle = selectedNote === index ? '#ffffff' : color; context.lineWidth = selectedNote === index ? 3 : 2; context.strokeRect(x, top, width, 38);
      context.fillStyle = color; context.fillRect(x, top, Math.min(26, width), 38);
      if (note.type === 'hold') { context.fillStyle = '#dce5ff'; context.font = '600 11px Arial'; context.fillText(`${note.tickCount} ticks`, x + 32, top + 24); }
    });
  }, [canvasHeight, canvasWidth, chart, endBeat, selectedNote]);

  const selectOrAddTap = (event: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current; if (!canvas) return;
    const bounds = canvas.getBoundingClientRect();
    const x = (event.clientX - bounds.left) * canvas.width / bounds.width;
    const y = (event.clientY - bounds.top) * canvas.height / bounds.height;
    if (x < 116 || y < 38) return;
    const laneIndex = Math.floor((y - 38) / laneHeight); if (laneIndex < 0 || laneIndex >= lanes.length) return;
    const beat = Math.max(0, Math.round((x - 116) / pixelsPerBeat)); const lane = lanes[laneIndex];
    const existing = chart.notes.findIndex((note) => note.type === 'tap' && note.lane === lane && Number(note.beat) === beat);
    if (existing >= 0) { setSelectedNote(existing); setNotice(`Selected ${lane} tap at beat ${beat}.`); return; }
    commit({ ...chart, notes: [...chart.notes, { type: 'tap', lane, beat: String(beat) }] }, `Added ${lane} tap at beat ${beat}.`);
  };

  const undo = () => {
    const previous = past.at(-1); if (!previous) { setNotice('Nothing to undo.'); return; }
    setPast((entries) => entries.slice(0, -1)); setFuture((entries) => [cloneChart(chart), ...entries]); setChart(previous); setSelectedNote(null); setNotice('Undid the last edit.');
  };
  const redo = () => {
    const next = future[0]; if (!next) { setNotice('Nothing to redo.'); return; }
    setFuture((entries) => entries.slice(1)); setPast((entries) => [...entries, cloneChart(chart)]); setChart(next); setSelectedNote(null); setNotice('Redid the edit.');
  };
  const removeSelected = () => {
    if (selectedNote === null) { setNotice('Select a tap note before removing it.'); return; }
    commit({ ...chart, notes: chart.notes.filter((_, index) => index !== selectedNote) }, 'Selected note removed.');
  };
  const loadText = (text: string, name: string, handle: FileHandleLike | null = null) => {
    try { replaceChart(parsePdxChart(text), `Loaded ${name}.`, name, handle); } catch (error) { setNotice(errorText(error)); }
  };
  const openChart = async () => {
    const picker = (window as typeof window & { showOpenFilePicker?: () => Promise<FileHandleLike[]> }).showOpenFilePicker;
    if (!picker) { inputRef.current?.click(); return; }
    try { const [handle] = await picker(); const file = await handle.getFile(); loadText(await file.text(), file.name, handle); }
    catch (error) { if (!(error instanceof DOMException && error.name === 'AbortError')) setNotice(errorText(error)); }
  };
  const onInputFile = async (event: ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0]; if (file) loadText(await file.text(), file.name); event.target.value = '';
  };
  const saveChart = async () => {
    try {
      const content = serializePdxChart(chart);
      if (fileHandle) { const writable = await fileHandle.createWritable(); await writable.write(content); await writable.close(); setNotice(`Saved ${fileName} in place.`); return; }
      const url = URL.createObjectURL(new Blob([content], { type: 'text/plain;charset=utf-8' }));
      const anchor = document.createElement('a'); anchor.href = url; anchor.download = fileName.endsWith('.pdxchart') ? fileName : `${fileName}.pdxchart`; anchor.click(); URL.revokeObjectURL(url);
      setNotice('Downloaded the compatible .pdxchart file. Edge can save directly into a selected file.');
    } catch (error) { setNotice(errorText(error)); }
  };

  return <main className="editor-shell">
    <header className="topbar"><div><p className="eyebrow">PUMPDX / STAGE 04</p><h1>Chart Editor</h1></div>
      <div className="topbar-actions"><button className="quiet-button" onClick={() => loadText(sampleChart, 'new-song-to-god-hold-playtest.pdxchart')}>Load sample</button><button className="quiet-button" onClick={openChart}>Open .pdxchart</button><button className="primary-button" onClick={saveChart}>Save chart</button><input ref={inputRef} className="hidden-input" type="file" accept=".pdxchart,text/plain" onChange={onInputFile} /></div>
    </header>
    <section className="editor-grid"><aside className="inspector">
      <section className="panel"><p className="panel-kicker">Chart metadata</p><label>Chart ID<input value={chart.id} onChange={(event) => setChart((current) => ({ ...current, id: event.target.value }))} /></label><label>Initial BPM<input type="number" min="1" value={chart.tempo[0]?.bpm ?? 120} onChange={(event) => { const bpm = Number(event.target.value); if (bpm > 0) setChart((current) => ({ ...current, tempo: [{ ...current.tempo[0], bpm }] })); }} /></label><p className="hint">State 21 edits one initial BPM. Tempo changes arrive with the advanced timing state.</p></section>
      <section className="panel"><p className="panel-kicker">Edit history</p><div className="button-row"><button className="quiet-button" onClick={undo} disabled={past.length === 0}>Undo</button><button className="quiet-button" onClick={redo} disabled={future.length === 0}>Redo</button></div><button className="danger-button" onClick={removeSelected}>Remove selected note</button></section>
      <section className="panel status-panel" aria-live="polite"><p className="panel-kicker">Editor status</p><p>{notice}</p></section>
    </aside><section className="workspace"><div className="workspace-heading"><div><p className="panel-kicker">5-panel timeline · 1 beat snap</p><h2>{fileName}</h2></div><span>{chart.notes.length} events</span></div>
      <div className="timeline-frame"><canvas ref={canvasRef} width={canvasWidth} height={canvasHeight} aria-label="Five lane chart timeline. Click an empty cell to add a tap note." onClick={selectOrAddTap} /></div>
      <div className="timeline-footer"><span>Click an empty lane cell to add a tap note.</span><span>Existing tap: click to select · Holds: preview only in this state.</span></div>
    </section></section>
  </main>;
}
