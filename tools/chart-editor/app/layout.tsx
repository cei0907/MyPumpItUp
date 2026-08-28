import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'PumpDX Chart Editor',
  description: 'Local browser editor for PumpDX .pdxchart source files.',
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return <html lang="en"><body>{children}</body></html>;
}
