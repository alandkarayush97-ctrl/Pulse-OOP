import { useState, useEffect } from "react";
import { Send } from "lucide-react";
import { api } from "../services/api";
import { useAuthStore } from "../store/authStore";

export default function Messages() {
  const { user } = useAuthStore();
  const [conversations, setConversations] = useState([]);
  const [active, setActive] = useState(null);
  const [thread, setThread] = useState([]);
  const [draft, setDraft] = useState("");
  const [loading, setLoading] = useState(true);
  const [searchQuery, setSearchQuery] = useState("");
  const [searchResults, setSearchResults] = useState([]);
  const [searching, setSearching] = useState(false);

  const runSearch = async (q) => {
    setSearchQuery(q);
    if (!q.trim()) { setSearchResults([]); return; }
    setSearching(true);
    try {
      const results = await api.searchUsers(q);
      setSearchResults(results.filter((u) => u.id !== user?.id));
    } finally {
      setSearching(false);
    }
  };

  const startConversation = (profile) => {
    setActive({ id: profile.id, name: profile.name });
    setThread([]);
    setSearchQuery("");
    setSearchResults([]);
    if (!conversations.some((c) => c.id === profile.id)) {
      setConversations([{ id: profile.id, name: profile.name, last_message: null }, ...conversations]);
    }
  };

  useEffect(() => {
    const loadConversations = () =>
      api.getConversations().then((list) => {
        setConversations(list);
        setActive((prev) => prev || (list.length ? list[0] : null));
      });
    loadConversations().finally(() => setLoading(false));
    const interval = setInterval(loadConversations, 8000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    if (!active) return;
    const load = () =>
      api.getThread(active.id).then(setThread).catch((err) => console.error("Thread load failed:", err.message));
    load();
    const interval = setInterval(load, 5000);
    return () => clearInterval(interval);
  }, [active]);

  const send = async () => {
    if (!draft.trim() || !active) return;
    const msg = await api.sendMessage(active.id, draft.trim());
    setThread([...thread, msg]);
    setDraft("");
  };

  if (loading) return <p className="text-muted text-sm p-8 text-center">Loading…</p>;

  return (
    <div className="grid grid-cols-[240px_1fr] h-screen">
      <div className="border-r border-border">
        <h1 className="text-xl font-bold px-4 py-3 border-b border-border">Messages</h1>
        <div className="p-3 border-b border-border relative">
          <input
            value={searchQuery}
            onChange={(e) => runSearch(e.target.value)}
            placeholder="Search people to message"
            className="w-full bg-surface border border-border rounded-full px-3 py-1.5 text-xs outline-none focus:border-accent"
          />
          {searchQuery.trim() && (
            <div className="absolute left-3 right-3 mt-1 bg-surface border border-border rounded-lg shadow-lg z-10 max-h-56 overflow-y-auto">
              {searching && <p className="text-xs text-muted p-2">Searching…</p>}
              {!searching && searchResults.length === 0 && (
                <p className="text-xs text-muted p-2">No users found.</p>
              )}
              {searchResults.map((u) => (
                <button
                  key={u.id}
                  onClick={() => startConversation(u)}
                  className="w-full text-left px-3 py-2 hover:bg-ink text-xs flex items-center gap-2"
                >
                  <div className="w-6 h-6 rounded-full bg-accent/20 flex items-center justify-center font-bold text-accent text-[10px]">
                    {u.name[0]}
                  </div>
                  <span>{u.name} <span className="text-muted">@{u.handle}</span></span>
                </button>
              ))}
            </div>
          )}
        </div>
        {conversations.map((c) => (
          <button
            key={c.id}
            onClick={() => setActive(c)}
            className={`w-full text-left px-4 py-3 border-b border-border hover:bg-surface/40 ${
              active?.id === c.id ? "bg-surface" : ""
            }`}
          >
            <p className="font-semibold text-sm flex items-center gap-1.5">
              {c.name}
              {c.unread_count > 0 && <span className="w-2 h-2 bg-accent rounded-full" />}
            </p>
            <p className="text-xs text-muted truncate">{c.last_message || "No messages yet"}</p>
          </button>
        ))}
      </div>

      {active ? (
        <div className="flex flex-col">
          <div className="px-4 py-3 border-b border-border font-semibold">{active.name}</div>
          <div className="flex-1 overflow-y-auto p-4 space-y-2">
            {thread.map((m) => (
              <div key={m.id} className={`flex ${m.sender_id === user?.id ? "justify-end" : "justify-start"}`}>
                <span
                  className={`px-3 py-2 rounded-2xl text-sm max-w-xs ${
                    m.sender_id === user?.id ? "bg-accent text-white" : "bg-surface border border-border"
                  }`}
                >
                  {m.content}
                </span>
              </div>
            ))}
          </div>
          <div className="p-3 border-t border-border flex gap-2">
            <input
              value={draft}
              onChange={(e) => setDraft(e.target.value)}
              onKeyDown={(e) => e.key === "Enter" && send()}
              placeholder="Start a message"
              className="flex-1 bg-surface border border-border rounded-full px-4 py-2 text-sm outline-none focus:border-accent"
            />
            <button onClick={send} className="bg-accent hover:bg-accentDim rounded-full p-2.5">
              <Send size={16} />
            </button>
          </div>
        </div>
      ) : (
        <div className="flex items-center justify-center text-muted text-sm">
          Search for someone on the left to start messaging.
        </div>
      )}
    </div>
  );
}
