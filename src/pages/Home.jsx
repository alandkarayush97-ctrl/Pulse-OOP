import { useAuthStore } from "../store/authStore";
import { useState, useEffect, useRef } from "react";
import Composer from "../components/Composer.jsx";
import TweetCard from "../components/TweetCard.jsx";
import { api } from "../services/api";

const VIBES = ["All", "Wholesome", "Tech & Deep Dive", "Debate/Spicy", "Muted Outrage"];
const PAGE_SIZE = 20;

function timeAgo(dateStr) {
  const diff = (Date.now() - new Date(dateStr + "Z")) / 1000;
  if (diff < 60) return "now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export default function Home() {
  const [vibe, setVibe] = useState("All");
  const [tweets, setTweets] = useState([]);
  const [loading, setLoading] = useState(true);
  const [loadingMore, setLoadingMore] = useState(false);
  const [hasMore, setHasMore] = useState(true);
  const [error, setError] = useState("");
  const [hasNew, setHasNew] = useState(false);
  const latestSeenId = useRef(0);

  const loadFeed = async () => {
    setLoading(true);
    setError("");
    try {
      const data = await api.getFeed(vibe, { limit: PAGE_SIZE, offset: 0 });
      setTweets(data);
      setHasMore(data.length === PAGE_SIZE);
      if (data.length) latestSeenId.current = data[0].id;
      setHasNew(false);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  const loadMore = async () => {
    setLoadingMore(true);
    try {
      const more = await api.getFeed(vibe, { limit: PAGE_SIZE, offset: tweets.length });
      setTweets((prev) => [...prev, ...more]);
      setHasMore(more.length === PAGE_SIZE);
    } finally {
      setLoadingMore(false);
    }
  };

  useEffect(() => { loadFeed(); }, [vibe]);

  // Poll for new posts from other users without disrupting scroll position.
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const { latestId } = await api.getLatestTweetId();
        if (latestId > latestSeenId.current) setHasNew(true);
      } catch {}
    }, 8000);
    return () => clearInterval(interval);
  }, []);

  const addTweet = async ({ content, energy, publishAt, coAuthorHandle, imageUrl }) => {
    try {
      await api.createTweet({ content, vibe: "Wholesome", energy, publishAt, coAuthorHandle, imageUrl });
      loadFeed();
    } catch (err) {
      setError(err.message);
    }
  };

  return (
    <div>
      <h1 className="text-xl font-bold px-4 py-3 border-b border-border sticky top-0 bg-ink/80 backdrop-blur z-10">
        Home
      </h1>

      <div className="flex gap-2 px-4 py-2 overflow-x-auto border-b border-border">
        {VIBES.map((v) => (
          <button
            key={v}
            onClick={() => setVibe(v)}
            className={`shrink-0 text-sm px-3 py-1.5 rounded-full border transition-colors ${
              vibe === v ? "bg-accent border-accent text-white" : "border-border text-muted hover:text-text"
            }`}
          >
            {v}
          </button>
        ))}
      </div>

      {!useAuthStore.getState().user?.isGuest && <Composer onPost={addTweet} />}

      {hasNew && (
        <button
          onClick={loadFeed}
          className="w-full text-center bg-accent/10 text-accent text-sm py-2 hover:bg-accent/20 border-b border-border"
        >
          New posts available — click to refresh
        </button>
      )}

      {error && <p className="text-red-400 text-sm px-4 py-2">{error}</p>}
      {loading ? (
        <p className="text-muted text-sm p-8 text-center">Loading…</p>
      ) : tweets.length === 0 ? (
        <p className="text-muted text-sm p-8 text-center">No posts match this vibe right now.</p>
      ) : (
        <>
          {tweets.map((t) => (
            <TweetCard key={t.id} tweet={{ ...t, time: timeAgo(t.createdAt) }} onChanged={loadFeed} />
          ))}
          {hasMore && (
            <button
              onClick={loadMore}
              disabled={loadingMore}
              className="w-full text-center text-accent text-sm py-3 hover:bg-surface/40 disabled:opacity-50"
            >
              {loadingMore ? "Loading…" : "Load more"}
            </button>
          )}
        </>
      )}
    </div>
  );
}
