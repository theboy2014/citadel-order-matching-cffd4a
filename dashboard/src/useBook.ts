import { useEffect, useState } from 'react';

export interface TopOfBook {
  bid: number;
  bidQty: number;
  ask: number;
  askQty: number;
}

export interface Trade {
  buyId: number;
  sellId: number;
  price: number;
  qty: number;
  ts: number;
}

const API = import.meta.env.VITE_API ?? 'http://localhost:8080';

// Poll the read-side API (which serves the Redis snapshot + recent trades)
// every 250ms. We never talk to the engine directly — only its mirrors.
export function useBook() {
  const [book, setBook] = useState<TopOfBook | null>(null);
  const [trades, setTrades] = useState<Trade[]>([]);

  useEffect(() => {
    let alive = true;
    const tick = async () => {
      try {
        const [b, t] = await Promise.all([
          fetch(`${API}/book`).then((r) => r.json()),
          fetch(`${API}/trades`).then((r) => r.json()),
        ]);
        if (!alive) return;
        setBook(b as TopOfBook);
        setTrades(t as Trade[]);
      } catch {
        // transient fetch error: keep the last good state, try again next tick
      }
    };
    tick();
    const id = setInterval(tick, 250);
    return () => {
      alive = false;
      clearInterval(id);
    };
  }, []);

  return { book, trades };
}
