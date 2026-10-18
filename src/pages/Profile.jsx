import { useState, useEffect, useRef } from "react";
import { useParams, Link } from "react-router-dom";
import { Calendar, Camera } from "lucide-react";
import TweetCard from "../components/TweetCard.jsx";
import { api } from "../services/api";
import { useAuthStore } from "../store/authStore";

function timeAgo(dateStr) {
  const diff = (Date.now() - new Date(dateStr + "Z")) / 1000;
  if (diff < 60) return "now";
  if (diff < 3600) return `${Math.floor(diff / 60)}m`;
  if (diff < 86400) return `${Math.floor(diff / 3600)}h`;
  return `${Math.floor(diff / 86400)}d`;
}

export default function Profile() {
  const { handle } = useParams();
  const { user, setAvatar } = useAuthStore();
  const [profile, setProfile] = useState(null);
  const [tweets, setTweets] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");
  const [following, setFollowing] = useState(false);
  const fileRef = useRef(null);

  const loadProfile = () => {
    setError("");
    Promise.all([api.getProfile(handle), api.getUserTweets(handle)])
      .then(([p, t]) => {
        setProfile(p);
        setFollowing(p.isFollowing);
        setTweets(t);
      })
      .catch((err) => setError(err.message))
      .finally(() => setLoading(false));
  };

  useEffect(() => {
    setLoading(true);
    loadProfile();
  }, [handle]);

  const toggleFollow = async () => {
    setFollowing(!following);
    try { await api.toggleFollow(handle); } catch {}
  };

  const handleAvatarFile = async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    if (file.size > 2 * 1024 * 1024) { alert("Image too large (max 2MB)"); return; }
    const reader = new FileReader();
    reader.onload = async () => {
      try {
        await api.updateAvatar(reader.result);
        setProfile((p) => ({ ...p, avatarUrl: reader.result }));
        setAvatar?.(reader.result);
      } catch {}
    };
    reader.readAsDataURL(file);
  };

  if (loading) return <p className="text-muted text-sm p-8 text-center">Loading…</p>;
  if (error) return <p className="text-red-400 text-sm p-8 text-center">{error}</p>;
  if (!profile) return null;

  return (
    <div>
      <div className="h-36 bg-gradient-to-r from-accent/40 to-surface" />
      <div className="px-4">
        <div className="flex items-start justify-between">
          <div className="relative -mt-10 w-20 h-20 rounded-full bg-ink border-4 border-ink overflow-hidden bg-accent/30 flex items-center justify-center text-2xl font-bold text-accent">
            {profile.avatarUrl ? (
              <img src={profile.avatarUrl} alt={profile.name} className="w-full h-full object-cover" />
            ) : (
              profile.name[0]?.toUpperCase()
            )}
            {profile.isMe && (
              <button
                onClick={() => fileRef.current?.click()}
                className="absolute inset-0 bg-ink/60 opacity-0 hover:opacity-100 flex items-center justify-center transition-opacity"
              >
                <Camera size={18} />
              </button>
            )}
            <input ref={fileRef} type="file" accept="image/*" onChange={handleAvatarFile} className="hidden" />
          </div>
          {!profile.isMe && (
            <button
              onClick={toggleFollow}
              className={`mt-3 text-sm font-semibold rounded-full px-4 py-1.5 ${
                following ? "border border-border text-text" : "bg-text text-ink"
              }`}
            >
              {following ? "Following" : "Follow"}
            </button>
          )}
        </div>
        <h1 className="text-xl font-bold mt-2">{profile.name}</h1>
        <p className="text-muted text-sm">@{profile.handle}</p>
        {profile.bio && <p className="mt-3 text-sm">{profile.bio}</p>}
        <p className="flex items-center gap-1 text-muted text-xs mt-2">
          <Calendar size={14} /> Joined {new Date(profile.createdAt).getFullYear()}
        </p>
        <div className="flex gap-4 mt-3 text-sm">
          <Link to={`/profile/${handle}/following`} className="hover:underline">
            <b>{profile.following}</b> <span className="text-muted">Following</span>
          </Link>
          <Link to={`/profile/${handle}/followers`} className="hover:underline">
            <b>{profile.followers}</b> <span className="text-muted">Followers</span>
          </Link>
        </div>
      </div>
      <div className="mt-4 border-t border-border">
        {tweets.length === 0 ? (
          <p className="text-muted text-sm p-8 text-center">No posts yet.</p>
        ) : (
          tweets.map((t) => <TweetCard key={t.id} tweet={{ ...t, time: timeAgo(t.createdAt) }} onChanged={loadProfile} />)
        )}
      </div>
    </div>
  );
}
