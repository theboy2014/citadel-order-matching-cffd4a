import { useBook } from './useBook';

export function OrderBook() {
  const { book, trades } = useBook();

  if (!book) return <div className="muted">connecting…</div>;

  const spread = book.ask && book.bid ? book.ask - book.bid : 0;

  return (
    <div className="board">
      <h1>Order Book</h1>
      <div className="tob">
        <div className="ask">
          <span className="label">ASK</span>
          <span className="px">{book.ask || '—'}</span>
          <span className="qty">{book.askQty}</span>
        </div>
        <div className="spread">spread {spread}</div>
        <div className="bid">
          <span className="label">BID</span>
          <span className="px">{book.bid || '—'}</span>
          <span className="qty">{book.bidQty}</span>
        </div>
      </div>

      <h2>Trade Tape</h2>
      <ul className="tape">
        {trades.map((t) => (
          <li key={t.ts}>
            <span className="px">{t.price}</span>
            <span className="qty">×{t.qty}</span>
            <span className="ids">
              {t.buyId}↔{t.sellId}
            </span>
          </li>
        ))}
      </ul>
    </div>
  );
}
