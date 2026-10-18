import { useAuthStore } from "../store/authStore";
import { useState, useEffect } from "react";
import { useParams, useNavigate } from "react-router-dom";
import { ArrowLeft } from "lucide-react";
import TweetCard from "../components/TweetCard.jsx";
import Composer from "../components/Composer.jsx";
import { api } from "../services/api";

function timeAgo(dateStr) {
  const diff = (Date.now() - new Date(dateStr + "Z")) / 1000;
  if (diff < 60) return "now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export default function Thread() {
  const { id } = useParams();
  const navigate = useNavigate();
  const [tweet, setTweet] = useState(null);
  const [replies, setReplies] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  const load = () => {
    setLoading(true);
    Promise.all([api.getTweet(id), api.getReplies(id)])
      .then(([t, r]) => { setTweet(t); setReplies(r); })
      .catch((err) => setError(err.message))
      .finally(() => setLoading(false));
  };

  useEffect(load, [id]);

  const addReply = async ({ content, energy, publishAt }) => {
    await api.createReply(id, { content, vibe: "Wholesome", energy, publishAt });
    load();
  };

  const onChanged = load;

  return (
    <div>
      <div className="flex items-center gap-3 px-4 py-3 border-b border-border sticky top-0 bg-ink/80 backdrop-blur z-10">
        <button onClick={() => navigate(-1)} className="hover:bg-surface rounded-full p-1.5">
          <ArrowLeft size={18} />
        </button>
        <h1 className="text-lg font-bold">Post</h1>
      </div>

      {loading && <p className="text-muted text-sm p-8 text-center">Loading…</p>}
      {error && <p className="text-red-400 text-sm p-8 text-center">{error}</p>}

      {tweet && <TweetCard tweet={{ ...tweet, time: timeAgo(tweet.createdAt) }} onChanged={onChanged} />}

      {!useAuthStore.getState().user?.isGuest && <Composer onPost={addReply} />}

      {replies.map((r) => (
        <TweetCard key={r.id} tweet={{ ...r, time: timeAgo(r.createdAt) }} onChanged={onChanged} />
      ))}
    </div>
  );
}
