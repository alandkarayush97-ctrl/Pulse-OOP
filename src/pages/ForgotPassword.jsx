import { useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import { api } from "../services/api";
import Logo from "../components/Logo.jsx";

export default function ForgotPassword() {
  const [step, setStep] = useState(1);
  const [email, setEmail] = useState("");
  const [code, setCode] = useState("");
  const [shownCode, setShownCode] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [done, setDone] = useState(false);
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();
  const input = "w-full bg-surface border border-border rounded-lg px-3 py-2.5 outline-none focus:border-accent";

  const run = async (fn) => {
    setError(""); setLoading(true);
    try { await fn(); } catch (e) { setError(e.message); } finally { setLoading(false); }
  };

  const sendCode = (e) => {
    e.preventDefault();
    if (!email) return setError("Enter your email.");
    run(async () => {
      const r = await api.forgotPassword(email);
      setShownCode(r.code || "");
      setStep(2);
    });
  };

  const reset = (e) => {
    e.preventDefault();
    if (password.length < 6) return setError("Password must be at least 6 characters.");
    run(async () => {
      await api.resetPassword({ email, code, password });
      setDone(true);
      setTimeout(() => navigate("/login"), 1500);
    });
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-ink px-4">
      <div className="w-full max-w-sm">
        <div className="flex items-center gap-2 justify-center mb-8">
          <Logo size={32} /><span className="text-2xl font-bold">Pulse</span>
        </div>
        <h1 className="text-2xl font-bold mb-6">Reset your password</h1>

        {done ? (
          <p className="text-green-500">Password updated. Redirecting to sign in…</p>
        ) : step === 1 ? (
          <form onSubmit={sendCode} className="space-y-4">
            <input type="email" value={email} onChange={(e) => setEmail(e.target.value)} className={input} placeholder="Your account email" />
            {error && <p className="text-red-400 text-sm">{error}</p>}
            <button disabled={loading} className="w-full bg-accent hover:bg-accentDim disabled:opacity-50 rounded-full py-2.5 font-semibold">
              {loading ? "Sending…" : "Get reset code"}
            </button>
          </form>
        ) : (
          <form onSubmit={reset} className="space-y-4">
            <div className="bg-surface border border-border rounded-lg p-3 text-sm">
              {shownCode ? (
                <>Demo mode (no email server). Your code: <b className="text-accent tracking-widest">{shownCode}</b></>
              ) : (
                <>If the email exists, a code was generated (check the server console).</>
              )}
            </div>
            <input value={code} onChange={(e) => setCode(e.target.value)} className={input} placeholder="6-digit code" inputMode="numeric" />
            <input type="password" value={password} onChange={(e) => setPassword(e.target.value)} className={input} placeholder="New password" />
            {error && <p className="text-red-400 text-sm">{error}</p>}
            <button disabled={loading} className="w-full bg-accent hover:bg-accentDim disabled:opacity-50 rounded-full py-2.5 font-semibold">
              {loading ? "Saving…" : "Set new password"}
            </button>
          </form>
        )}
        <p className="text-muted text-sm text-center mt-6">
          <Link to="/login" className="text-accent hover:underline">Back to sign in</Link>
        </p>
      </div>
    </div>
  );
}
