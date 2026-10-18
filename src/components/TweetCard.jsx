import { useState } from "react";
import { Link } from "react-router-dom";
import { MessageCircle, Repeat2, Heart, Share, ShieldCheck, Lock, Pencil, Trash2, Users } from "lucide-react";
import { api } from "../services/api";
import { useAuthStore } from "../store/authStore";
import FactCheckDrawer from "./FactCheckDrawer.jsx";

const ENERGY_COLORS = {
  "Venting": "bg-red-500/15 text-red-400",
  "Celebrating": "bg-yellow-500/15 text-yellow-400",
  "Asking advice": "bg-blue-500/15 text-blue-400",
  "Just sharing": "bg-gray-500/15 text-muted",
};

const EMOJIS = ["👍", "❤️", "😂", "😮", "🔥"];

export default function TweetCard({ tweet, onChanged }) {
  const { user } = useAuthStore();
  const [liked, setLiked] = useState(tweet.likedByMe);
  const [likes, setLikes] = useState(tweet.likes);
  const [retweeted, setRetweeted] = useState(false);
  const [retweets, setRetweets] = useState(tweet.retweets);
  const [showPicker, setShowPicker] = useState(false);
  const [myReaction, setMyReaction] = useState(tweet.myReaction);
  const [reactions, setReactions] = useState(tweet.reactions || []);
  const [showFactCheck, setShowFactCheck] = useState(false);
  const [editing, setEditing] = useState(false);
  const [editText, setEditText] = useState(tweet.content);
  const [content, setContent] = useState(tweet.content);

  const isLocked = tweet.publishAt && new Date(tweet.publishAt) > new Date();
  const isMine = user?.id === tweet.author.id;

  const toggleLike = async () => {
    setLiked(!liked);
    setLikes(liked ? likes - 1 : likes + 1);
    try { await api.likeTweet(tweet.id); } catch {}
  };

  const toggleRetweet = async () => {
    setRetweeted(!retweeted);
    setRetweets(retweeted ? retweets - 1 : retweets + 1);
    try { await api.retweetTweet(tweet.id); } catch {}
  };

  const pickEmoji = async (emoji) => {
    const next = myReaction === emoji ? null : emoji;
    setMyReaction(next);
    setShowPicker(false);
    try {
      await api.reactTweet(tweet.id, emoji);
      setReactions((prev) => {
        const base = prev.filter((r) => r.emoji !== myReaction).map((r) =>
          r.emoji === emoji ? { ...r, count: r.count + (next ? 1 : 0) } : r
        );
        if (next && !base.some((r) => r.emoji === emoji)) base.push({ emoji, count: 1 });
        return base;
      });
    } catch {}
  };

  const saveEdit = async () => {
    if (!editText.trim()) return;
    try {
      await api.updateTweet(tweet.id, editText.trim());
      setContent(editText.trim());
      setEditing(false);
    } catch {}
  };

  const remove = async () => {
    if (!confirm("Delete this post?")) return;
    try {
      await api.deleteTweet(tweet.id);
      onChanged?.();
    } catch {}
  };

  const pendingCoauthorForMe = tweet.coAuthors?.find((c) => c.id === user?.id && c.status === "pending");
  const respondCoauthor = async (accept) => {
    try {
      await api.respondCoauthor(tweet.id, accept);
      onChanged?.();
    } catch {}
  };

  return (
    <article className="border-b border-border px-4 py-3 hover:bg-surface/40 transition-colors">
      <div className="flex gap-3">
        <div className="w-11 h-11 rounded-full bg-accent/20 flex items-center justify-center font-bold text-accent shrink-0">
          {tweet.author.name[0]}
        </div>
        <div className="flex-1 min-w-0">
          <div className="flex items-center gap-1 text-sm flex-wrap">
            <Link to={`/profile/${tweet.author.handle}`} className="font-bold hover:underline">{tweet.author.name}</Link>
            <span className="text-muted">@{tweet.author.handle} · {tweet.time}</span>
            {tweet.energy && (
              <span className={`text-[11px] px-2 py-0.5 rounded-full ${ENERGY_COLORS[tweet.energy] || ENERGY_COLORS["Just sharing"]}`}>
                {tweet.energy}
              </span>
            )}
            {isLocked && (
              <span className="text-[11px] px-2 py-0.5 rounded-full bg-accent/15 text-accent flex items-center gap-1">
                <Lock size={10} /> Unlocks {new Date(tweet.publishAt).toLocaleDateString()}
              </span>
            )}
            {tweet.coAuthors?.some((c) => c.status === "accepted") && (
              <span className="text-[11px] px-2 py-0.5 rounded-full bg-accent/15 text-accent flex items-center gap-1">
                <Users size={10} /> with @{tweet.coAuthors.find((c) => c.status === "accepted")?.handle}
              </span>
            )}
            {isMine && (
              <span className="ml-auto flex gap-2">
                <button onClick={() => setEditing(!editing)} className="text-muted hover:text-accent"><Pencil size={14} /></button>
                <button onClick={remove} className="text-muted hover:text-red-400"><Trash2 size={14} /></button>
              </span>
            )}
          </div>

          {pendingCoauthorForMe && (
            <div className="mt-2 bg-accent/10 border border-accent/30 rounded-lg p-2 text-xs flex items-center justify-between">
              <span>Invited to co-author this post</span>
              <span className="flex gap-2">
                <button onClick={() => respondCoauthor(true)} className="text-accent font-semibold">Accept</button>
                <button onClick={() => respondCoauthor(false)} className="text-muted">Decline</button>
              </span>
            </div>
          )}

          {editing ? (
            <div className="mt-1">
              <textarea
                value={editText}
                onChange={(e) => setEditText(e.target.value)}
                className="w-full bg-surface border border-border rounded-lg p-2 text-sm outline-none focus:border-accent"
                rows={2}
              />
              <div className="flex gap-2 mt-1">
                <button onClick={saveEdit} className="text-xs bg-accent text-white rounded-full px-3 py-1">Save</button>
                <button onClick={() => setEditing(false)} className="text-xs text-muted">Cancel</button>
              </div>
            </div>
          ) : (
            <Link to={`/tweet/${tweet.id}`}>
              <p className="mt-1 text-[15px] leading-normal whitespace-pre-wrap hover:opacity-90">{content}</p>
              {tweet.imageUrl?.startsWith("data:video") && (
                <video src={tweet.imageUrl} controls className="mt-2 rounded-xl border border-border max-h-80 w-full" />
              )}
              {tweet.imageUrl?.startsWith("data:audio") && (
                <audio src={tweet.imageUrl} controls className="mt-2 w-full" />
              )}
              {tweet.imageUrl?.startsWith("data:image") && (
                <img src={tweet.imageUrl} alt="post attachment" className="mt-2 rounded-xl border border-border max-h-80 w-full object-cover" />
              )}
            </Link>
          )}

          <div className="flex items-center justify-between max-w-md mt-3 text-muted">
            <Link to={`/tweet/${tweet.id}`} className="flex items-center gap-1 hover:text-accent text-xs">
              <MessageCircle size={16} /> {tweet.replies}
            </Link>
            <button
              onClick={toggleRetweet}
              className={`flex items-center gap-1 text-xs hover:text-green-500 ${retweeted ? "text-green-500" : ""}`}
            >
              <Repeat2 size={16} /> {retweets}
            </button>
            <button
              onClick={toggleLike}
              className={`flex items-center gap-1 text-xs hover:text-pink-500 ${liked ? "text-pink-500" : ""}`}
            >
              <Heart size={16} fill={liked ? "currentColor" : "none"} /> {likes}
            </button>
            <button onClick={() => setShowFactCheck(true)} className="flex items-center gap-1 hover:text-accent text-xs">
              <ShieldCheck size={16} /> Check
            </button>
            <button className="hover:text-accent"><Share size={16} /></button>
          </div>

          <div className="relative mt-2">
            <div className="flex gap-1 flex-wrap">
              {reactions.map((r) => (
                <span key={r.emoji} className="text-xs bg-surface border border-border rounded-full px-2 py-0.5">
                  {r.emoji} {r.count}
                </span>
              ))}
              <button
                onClick={() => setShowPicker(!showPicker)}
                className="text-xs border border-border rounded-full px-2 py-0.5 text-muted hover:text-text"
              >
                {myReaction || "➕"}
              </button>
            </div>
            {showPicker && (
              <div className="absolute bottom-full mb-1 bg-surface border border-border rounded-full px-2 py-1 flex gap-1 shadow-lg z-10">
                {EMOJIS.map((e) => (
                  <button key={e} onClick={() => pickEmoji(e)} className="text-lg hover:scale-125 transition-transform">
                    {e}
                  </button>
                ))}
              </div>
            )}
          </div>
        </div>
      </div>

      <FactCheckDrawer tweetId={tweet.id} isOpen={showFactCheck} onClose={() => setShowFactCheck(false)} />
    </article>
  );
}
