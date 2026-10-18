import { useState } from "react";
import { Link } from "react-router-dom";
import { Search } from "lucide-react";
import { api } from "../services/api";
import TweetCard from "../components/TweetCard.jsx";

const trends = [
  { tag: "#DesignSystems", posts: "42.1K posts" },
  { tag: "#AIethics", posts: "18.7K posts" },
  { tag: "#OpenSourceWeek", posts: "9.3K posts" },
  { tag: "#IndieHackers", posts: "6.5K posts" },
];

function timeAgo(dateStr) {
  const diff = (Date.now() - new Date(dateStr + "Z")) / 1000;
  if (diff < 60) return "now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export default function Explore() {
  const [query, setQuery] = useState("");
  const [tab, setTab] = useState("people");
  const [userResults, setUserResults] = useState([]);
  const [tweetResults, setTweetResults] = useState([]);
  const [searching, setSearching] = useState(false);

  const runSearch = async (q, activeTab = tab) => {
    setQuery(q);
    if (!q.trim()) { setUserResults([]); setTweetResults([]); return; }
    setSearching(true);
    try {
      if (activeTab === "people") setUserResults(await api.searchUsers(q));
      else setTweetResults(await api.searchTweets(q));
    } finally {
      setSearching(false);
    }
  };

  const switchTab = (t) => {
    setTab(t);
    if (query.trim()) runSearch(query, t);
  };

  return (
    <div>
      <div className="p-4 border-b border-border sticky top-0 bg-ink/80 backdrop-blur z-10">
        <div className="flex items-center gap-2 bg-surface rounded-full px-4 py-2 border border-border">
          <Search size={18} className="text-muted" />
          <input
            value={query}
            onChange={(e) => runSearch(e.target.value)}
            placeholder="Search people or posts"
            className="bg-transparent outline-none flex-1 text-sm placeholder:text-muted"
          />
        </div>
        {query.trim() && (
          <div className="flex gap-2 mt-3">
            {["people", "posts"].map((t) => (
              <button
                key={t}
                onClick={() => switchTab(t)}
                className={`text-xs px-3 py-1.5 rounded-full border capitalize ${
                  tab === t ? "bg-accent border-accent text-white" : "border-border text-muted"
                }`}
              >
                {t}
              </button>
            ))}
          </div>
        )}
      </div>

      {query.trim() ? (
        <>
          {searching && <p className="text-muted text-sm p-4">Searching…</p>}
          {tab === "people" && !searching && (
            userResults.length === 0 ? (
              <p className="text-muted text-sm p-4">No users found for "{query}".</p>
            ) : (
              userResults.map((u) => (
                <Link key={u.id} to={`/profile/${u.handle}`} className="flex items-center gap-3 px-4 py-3 border-b border-border hover:bg-surface/40">
                  <div className="w-10 h-10 rounded-full bg-accent/20 flex items-center justify-center font-bold text-accent">{u.name[0]}</div>
                  <div>
                    <p className="font-semibold text-sm">{u.name}</p>
                    <p className="text-muted text-xs">@{u.handle}</p>
                  </div>
                </Link>
              ))
            )
          )}
          {tab === "posts" && !searching && (
            tweetResults.length === 0 ? (
              <p className="text-muted text-sm p-4">No posts found for "{query}".</p>
            ) : (
              tweetResults.map((t) => <TweetCard key={t.id} tweet={{ ...t, time: timeAgo(t.createdAt) }} />)
            )
          )}
        </>
      ) : (
        <>
          <h2 className="px-4 py-3 font-bold text-lg">Trends for you</h2>
          {trends.map((t) => (
            <div key={t.tag} className="px-4 py-3 border-b border-border hover:bg-surface/40">
              <p className="text-xs text-muted">Trending</p>
              <p className="font-semibold">{t.tag}</p>
              <p className="text-xs text-muted">{t.posts}</p>
            </div>
          ))}
        </>
      )}
    </div>
  );
}
