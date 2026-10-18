import { useState, useEffect } from "react";
import { Heart, Repeat2, UserPlus, MessageCircle } from "lucide-react";
import { api } from "../services/api";

const ICONS = {
  like: { icon: Heart, color: "text-pink-500" },
  retweet: { icon: Repeat2, color: "text-green-500" },
  follow: { icon: UserPlus, color: "text-accent" },
  reply: { icon: MessageCircle, color: "text-accent" },
};

function timeAgo(dateStr) {
  const diff = (Date.now() - new Date(dateStr + "Z")) / 1000;
  if (diff < 60) return "now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export default function Notifications() {
  const [items, setItems] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  useEffect(() => {
    const load = () => api.getNotifications().then(setItems).catch((err) => setError(err.message));
    load();
    api.markNotificationsRead().catch(() => {});
    setLoading(false);
    const interval = setInterval(load, 10000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div>
      <h1 className="text-xl font-bold px-4 py-3 border-b border-border sticky top-0 bg-ink/80 backdrop-blur z-10">
        Notifications
      </h1>
      {loading && <p className="text-muted text-sm p-8 text-center">Loading…</p>}
      {error && <p className="text-red-400 text-sm p-8 text-center">{error}</p>}
      {!loading && items.length === 0 && (
        <p className="text-muted text-sm p-8 text-center">No notifications yet.</p>
      )}
      {items.map((n) => {
        const { icon: Icon, color } = ICONS[n.type] || ICONS.like;
        return (
          <div key={n.id} className={`flex items-center gap-3 px-4 py-4 border-b border-border hover:bg-surface/40 ${!n.read ? "bg-accent/5" : ""}`}>
            <Icon className={color} size={22} />
            <p className="text-sm flex-1">{n.message}</p>
            <span className="text-xs text-muted">{timeAgo(n.createdAt)}</span>
          </div>
        );
      })}
    </div>
  );
}
