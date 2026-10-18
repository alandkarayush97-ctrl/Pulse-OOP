import { useState } from "react";
import { X, ShieldCheck, Loader2 } from "lucide-react";
import { api } from "../services/api";

export default function FactCheckDrawer({ tweetId, isOpen, onClose }) {
  const [data, setData] = useState(null);
  const [loading, setLoading] = useState(false);

  const analyze = async () => {
    setLoading(true);
    try {
      const result = await api.factCheckTweet(tweetId);
      setData(result);
    } finally {
      setLoading(false);
    }
  };

  if (!isOpen) return null;

  const color = !data ? "" : data.neutralityScore >= 70 ? "bg-green-500" : data.neutralityScore >= 40 ? "bg-yellow-500" : "bg-red-500";

  return (
    <div className="fixed right-0 top-0 h-full w-80 bg-ink border-l border-border shadow-xl z-50 flex flex-col">
      <div className="flex justify-between items-center p-4 border-b border-border">
        <h2 className="font-bold flex items-center gap-2">
          <ShieldCheck size={18} className="text-accent" /> Context Check
        </h2>
        <button onClick={onClose}><X size={18} /></button>
      </div>

      <div className="p-4 flex-1 overflow-y-auto">
        {loading && <Loader2 className="animate-spin mx-auto mt-8" />}
        {!data && !loading && (
          <button onClick={analyze} className="w-full bg-accent text-white rounded-full py-2 font-medium">
            Analyze this post
          </button>
        )}
        {data && (
          <>
            <div className="flex justify-between text-xs mb-1">
              <span>Neutrality Score</span><span>{data.neutralityScore}/100</span>
            </div>
            <div className="w-full h-2 bg-surface rounded-full">
              <div className={`${color} h-2 rounded-full`} style={{ width: `${data.neutralityScore}%` }} />
            </div>
            <p className="text-sm mt-4 text-muted">{data.summary}</p>
            {data.flaggedWords.length > 0 && (
              <div className="mt-4">
                <h3 className="text-xs font-semibold mb-2 text-muted">FLAGGED LANGUAGE</h3>
                <div className="flex gap-1.5 flex-wrap">
                  {data.flaggedWords.map((w, i) => (
                    <span key={i} className="text-xs bg-surface border border-border rounded-full px-2 py-0.5">{w}</span>
                  ))}
                </div>
              </div>
            )}
            <p className="text-[11px] text-muted mt-4 italic">{data.note}</p>
          </>
        )}
      </div>
    </div>
  );
}
