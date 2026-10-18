import { useState } from "react";
import { useNavigate, Link } from "react-router-dom";
import { useAuthStore } from "../store/authStore";
import Logo from "../components/Logo.jsx";

export default function Login() {
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);
  const login = useAuthStore((s) => s.login);
  const guestLogin = useAuthStore((s) => s.guestLogin);
  const navigate = useNavigate();

  const guest = async () => {
    setError(""); setLoading(true);
    try { await guestLogin(); navigate("/"); } catch (err) { setError(err.message); } finally { setLoading(false); }
  };

  const submit = async (e) => {
    e.preventDefault();
    setError("");
    if (!email || !password) return setError("Enter your email and password.");
    setLoading(true);
    try {
      await login(email, password);
      navigate("/");
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-ink px-4">
      <div className="w-full max-w-sm">
        <div className="flex items-center gap-2 justify-center mb-8">
          <Logo size={32} />
          <span className="text-2xl font-bold">Pulse</span>
        </div>

        <h1 className="text-2xl font-bold mb-6">Sign in to Pulse</h1>

        <form onSubmit={submit} className="space-y-4">
          <div>
            <label className="text-sm text-muted block mb-1">Email</label>
            <input
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              className="w-full bg-surface border border-border rounded-lg px-3 py-2.5 outline-none focus:border-accent"
              placeholder="you@example.com"
            />
          </div>
          <div>
            <div className="flex justify-between mb-1">
              <label className="text-sm text-muted">Password</label>
              <Link to="/forgot" className="text-sm text-accent hover:underline">Forgot password?</Link>
            </div>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              className="w-full bg-surface border border-border rounded-lg px-3 py-2.5 outline-none focus:border-accent"
              placeholder="••••••••"
            />
          </div>

          {error && <p className="text-red-400 text-sm">{error}</p>}

          <button
            type="submit"
            disabled={loading}
            className="w-full bg-accent hover:bg-accentDim disabled:opacity-50 rounded-full py-2.5 font-semibold"
          >
            {loading ? "Signing in…" : "Sign in"}
          </button>
        </form>

        <button
          type="button"
          onClick={guest}
          disabled={loading}
          className="w-full mt-3 border border-border hover:bg-surface disabled:opacity-50 rounded-full py-2.5 font-semibold"
        >
          Continue as guest
        </button>
        <p className="text-muted text-xs text-center mt-2">Guests can browse only: no posting, liking, following or messages.</p>

        <p className="text-muted text-sm text-center mt-6">
          New to Pulse?{" "}
          <Link to="/signup" className="text-accent hover:underline">
            Create an account
          </Link>
        </p>
      </div>
    </div>
  );
}
