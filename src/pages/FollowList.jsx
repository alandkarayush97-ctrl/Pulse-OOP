import { useState, useEffect } from "react";
import { useParams, Link, useNavigate } from "react-router-dom";
import { ArrowLeft } from "lucide-react";
import { api } from "../services/api";

export default function FollowList({ mode }) {
  const { handle } = useParams();
  const navigate = useNavigate();
  const [users, setUsers] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetcher = mode === "followers" ? api.getFollowers : api.getFollowing;
    fetcher(handle).then(setUsers).finally(() => setLoading(false));
  }, [handle, mode]);

  return (
    <div>
      <div className="flex items-center gap-3 px-4 py-3 border-b border-border sticky top-0 bg-ink/80 backdrop-blur z-10">
        <button onClick={() => navigate(-1)} className="hover:bg-surface rounded-full p-1.5">
          <ArrowLeft size={18} />
        </button>
        <h1 className="text-lg font-bold capitalize">@{handle} · {mode}</h1>
      </div>

      {loading && <p className="text-muted text-sm p-8 text-center">Loading…</p>}
      {!loading && users.length === 0 && (
        <p className="text-muted text-sm p-8 text-center">No {mode} yet.</p>
      )}
      {users.map((u) => (
        <Link
          key={u.id}
          to={`/profile/${u.handle}`}
          className="flex items-center gap-3 px-4 py-3 border-b border-border hover:bg-surface/40"
        >
          <div className="w-10 h-10 rounded-full bg-accent/20 flex items-center justify-center font-bold text-accent">
            {u.name[0]}
          </div>
          <div>
            <p className="font-semibold text-sm">{u.name}</p>
            <p className="text-muted text-xs">@{u.handle}</p>
          </div>
        </Link>
      ))}
    </div>
  );
}
