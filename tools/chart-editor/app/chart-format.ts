export const lanes = ['SW', 'NW', 'C', 'NE', 'SE'] as const;

export type Lane = (typeof lanes)[number];

export type TapNote = {
  type: 'tap';
  lane: Lane;
  beat: string;
};

export type HoldNote = {
  type: 'hold';
  lane: Lane;
  startBeat: string;
  endBeat: string;
  tickCount: number;
};

export type ChartNote = TapNote | HoldNote;

export type ChartDocument = {
  id: string;
  tempo: Array<{ beat: string; bpm: number }>;
  notes: ChartNote[];
};

const defaultChart: ChartDocument = {
  id: 'untitled-chart',
  tempo: [{ beat: '0', bpm: 120 }],
  notes: [],
};

const isLane = (value: string): value is Lane => lanes.includes(value as Lane);

const trim = (value: string) => value.trim();

export function cloneChart(chart: ChartDocument): ChartDocument {
  return {
    id: chart.id,
    tempo: chart.tempo.map((segment) => ({ ...segment })),
    notes: chart.notes.map((note) => ({ ...note })),
  };
}

export function emptyChart(): ChartDocument {
  return cloneChart(defaultChart);
}

export function beatToNumber(value: string): number {
  const text = trim(value);
  const plus = text.indexOf('+');
  if (plus > 0) {
    return beatToNumber(text.slice(0, plus)) + beatToNumber(text.slice(plus + 1));
  }
  const slash = text.indexOf('/');
  if (slash > 0) {
    const numerator = Number(text.slice(0, slash));
    const denominator = Number(text.slice(slash + 1));
    if (!Number.isFinite(numerator) || !Number.isFinite(denominator) || denominator === 0) {
      return Number.NaN;
    }
    return numerator / denominator;
  }
  return Number(text);
}

function requireBeat(value: string, line: number): string {
  if (!Number.isFinite(beatToNumber(value))) {
    throw new Error(`Line ${line}: beat must be an integer, fraction, or mixed fraction.`);
  }
  return value;
}

export function parsePdxChart(source: string): ChartDocument {
  let section: 'header' | 'tempo' | 'notes' = 'header';
  let sawVersion = false;
  const chart = emptyChart();
  chart.tempo = [];

  source.split(/\r?\n/).forEach((rawLine, index) => {
    const lineNumber = index + 1;
    const line = trim(rawLine);
    if (!line || line.startsWith('#')) {
      return;
    }
    if (line === '[tempo]') {
      section = 'tempo';
      return;
    }
    if (line === '[notes]') {
      section = 'notes';
      return;
    }

    const separator = line.indexOf('=');
    if (separator < 1) {
      throw new Error(`Line ${lineNumber}: expected key=value syntax.`);
    }
    const key = trim(line.slice(0, separator));
    const value = trim(line.slice(separator + 1));
    if (!value) {
      throw new Error(`Line ${lineNumber}: a value is required.`);
    }

    if (section === 'header') {
      if (key === 'schemaVersion' && value === '1' && !sawVersion) {
        sawVersion = true;
        return;
      }
      if (key === 'id' && chart.id === 'untitled-chart') {
        chart.id = value;
        return;
      }
      throw new Error(`Line ${lineNumber}: unknown or duplicate header value.`);
    }

    if (section === 'tempo') {
      const bpm = Number(value);
      if (!Number.isFinite(bpm) || bpm <= 0) {
        throw new Error(`Line ${lineNumber}: BPM must be a positive number.`);
      }
      chart.tempo.push({ beat: requireBeat(key, lineNumber), bpm });
      return;
    }

    const fields = value.split(',').map(trim);
    if (key === 'tap' && fields.length === 2 && isLane(fields[0])) {
      chart.notes.push({ type: 'tap', lane: fields[0], beat: requireBeat(fields[1], lineNumber) });
      return;
    }
    if (key === 'hold' && fields.length === 4 && isLane(fields[0])) {
      const tickCount = Number(fields[3]);
      const startBeat = requireBeat(fields[1], lineNumber);
      const endBeat = requireBeat(fields[2], lineNumber);
      if (!Number.isInteger(tickCount) || tickCount < 1 || beatToNumber(endBeat) <= beatToNumber(startBeat)) {
        throw new Error(`Line ${lineNumber}: hold requires an end beat after its start and a positive tick count.`);
      }
      chart.notes.push({ type: 'hold', lane: fields[0], startBeat, endBeat, tickCount });
      return;
    }
    throw new Error(`Line ${lineNumber}: expected tap=LANE,BEAT or hold=LANE,START,END,TICKS.`);
  });

  if (!sawVersion || chart.id === 'untitled-chart' || chart.tempo.length === 0) {
    throw new Error('A chart needs schemaVersion=1, id, and at least one tempo entry.');
  }
  chart.tempo.sort((left, right) => beatToNumber(left.beat) - beatToNumber(right.beat));
  chart.notes.sort((left, right) => {
    const leftBeat = left.type === 'tap' ? left.beat : left.startBeat;
    const rightBeat = right.type === 'tap' ? right.beat : right.startBeat;
    return beatToNumber(leftBeat) - beatToNumber(rightBeat);
  });
  return chart;
}

export function serializePdxChart(chart: ChartDocument): string {
  const id = trim(chart.id);
  if (!id) {
    throw new Error('Chart id cannot be empty.');
  }
  if (chart.tempo.length === 0) {
    throw new Error('At least one tempo entry is required.');
  }

  const tempo = [...chart.tempo].sort((left, right) => beatToNumber(left.beat) - beatToNumber(right.beat));
  const notes = [...chart.notes].sort((left, right) => {
    const leftBeat = left.type === 'tap' ? left.beat : left.startBeat;
    const rightBeat = right.type === 'tap' ? right.beat : right.startBeat;
    return beatToNumber(leftBeat) - beatToNumber(rightBeat);
  });
  const lines = ['schemaVersion=1', `id=${id}`, '', '[tempo]'];
  tempo.forEach((segment) => lines.push(`${segment.beat}=${segment.bpm}`));
  lines.push('', '[notes]');
  notes.forEach((note) => {
    lines.push(note.type === 'tap'
      ? `tap=${note.lane},${note.beat}`
      : `hold=${note.lane},${note.startBeat},${note.endBeat},${note.tickCount}`);
  });
  return `${lines.join('\n')}\n`;
}

export function chartEndBeat(chart: ChartDocument): number {
  return Math.max(16, ...chart.notes.map((note) => beatToNumber(note.type === 'tap' ? note.beat : note.endBeat)));
}
