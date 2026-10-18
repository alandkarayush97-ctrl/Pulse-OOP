import { useState, useEffect } from "react";
import { Outlet, NavLink, useNavigate, Link } from "react-router-dom";
import {
  Home, Search, Bell, Mail, User, Feather, LogOut, Sun, Moon,
} from "lucide-react";
import { useAuthStore } from "../store/authStore";
import { useThemeStore } from "../store/themeStore";
import { api } from "../services/api";
import Logo from "./Logo.jsx";

const navItems = [
  { to: "/", label: "Home", icon: Home, end: true },
  { to: "/explore", label: "Explore", icon: Search },
  { to: "/notifications", label: "Notifications", icon: Bell },
  { to: "/messages", label: "Messages", icon: Mail, badge: true },
];

export default function AppLayout() {
  const { user, logout } = useAuthStore();
  const { isLight, toggleTheme } = useThemeStore();
  const navigate = useNavigate();
  const isGuest = !!user?.isGuest;
  const [trending, setTrending] = useState([]);
  const [suggestions, setSuggestions] = useState([]);
  const [unreadMessages, setUnreadMessages] = useState(0);

  useEffect(() => {
    api.getTrending().then(setTrending).catch(() => {});
    if (isGuest) return;
    api.getSuggestions().then(setSuggestions).catch(() => {});
    const checkUnread = () => api.getUnreadMessageCount().then((r) => setUnreadMessages(r.count)).catch(() => {});
    checkUnread();
    const interval = setInterval(checkUnread, 8000);
    return () => clearInterval(interval);
  }, []);

  const visibleNav = isGuest ? navItems.filter((n) => n.to === "/" || n.to === "/explore") : navItems;

  const follow = async (handle, id) => {
    setSuggestions((prev) => prev.filter((s) => s.id !== id));
    try { await api.toggleFollow(handle); } catch {}
  };

  return (
    <div className="min-h-screen mx-auto max-w-6xl grid grid-cols-[72px_1fr] md:grid-cols-[240px_1fr_320px]">
      <aside className="border-r border-border flex flex-col justify-between py-4 px-2 md:px-4 sticky top-0 h-screen">
        <div className="space-y-2">
          <div className="flex items-center gap-2 px-2 mb-6">
            <Logo size={26} />
            <span className="hidden md:inline text-xl font-bold">Pulse</span>
          </div>
          {visibleNav.map(({ to, label, icon: Icon, end, badge }) => (
            <NavLink
              key={to}
              to={to}
              end={end}
              className={({ isActive }) =>
                `relative flex items-center gap-3 px-3 py-3 rounded-full transition-colors ${
                  isActive ? "bg-surface text-accent font-semibold" : "text-muted hover:bg-surface hover:text-text"
                }`
              }
            >
              <span className="relative">
                <Icon size={22} />
                {badge && unreadMessages > 0 && (
                  <span className="absolute -top-1 -right-1 w-2.5 h-2.5 bg-accent rounded-full border-2 border-ink" />
                )}
              </span>
              <span className="hidden md:inline">{label}</span>
            </NavLink>
          ))}
          {!isGuest && <NavLink
            to={`/profile/${user?.handle?.replace("@", "")}`}
            className={({ isActive }) =>
              `flex items-center gap-3 px-3 py-3 rounded-full transition-colors ${
                isActive ? "bg-surface text-accent font-semibold" : "text-muted hover:bg-surface hover:text-text"
              }`
            }
          >
            <User size={22} />
            <span className="hidden md:inline">Profile</span>
          </NavLink>}
          {!isGuest && <button className="w-full mt-4 bg-accent hover:bg-accentDim text-white rounded-full py-3 font-semibold flex items-center justify-center gap-2">
            <Feather size={18} />
            <span className="hidden md:inline">Post</span>
          </button>}
        </div>

        <div className="space-y-1">
          <button
            onClick={toggleTheme}
            className="w-full flex items-center gap-3 px-3 py-3 rounded-full text-muted hover:bg-surface hover:text-text"
          >
            {isLight ? <Moon size={20} /> : <Sun size={20} />}
            <span className="hidden md:inline">{isLight ? "Dark mode" : "Light mode"}</span>
          </button>
          <button
            onClick={() => { logout(); navigate("/login"); }}
            className="w-full flex items-center gap-3 px-3 py-3 rounded-full text-muted hover:bg-surface hover:text-text"
          >
            <LogOut size={20} />
            <span className="hidden md:inline">Log out</span>
          </button>
        </div>
      </aside>

      <main className="border-r border-border min-h-screen">
        {isGuest && (
          <div className="bg-surface border-b border-border px-4 py-2 text-sm flex justify-between items-center">
            <span className="text-muted">Guest mode: read-only.</span>
            <Link to="/signup" onClick={logout} className="text-accent font-semibold hover:underline">Create account</Link>
          </div>
        )}
        <Outlet />
      </main>

      <nav className="sm:hidden fixed bottom-0 left-0 right-0 bg-ink border-t border-border flex justify-around py-2 z-40">
        {visibleNav.map(({ to, icon: Icon, end, badge }) => (
          <NavLink
            key={to}
            to={to}
            end={end}
            className={({ isActive }) => `relative p-2 rounded-full ${isActive ? "text-accent" : "text-muted"}`}
          >
            <Icon size={22} />
            {badge && unreadMessages > 0 && (
              <span className="absolute top-1 right-1 w-2 h-2 bg-accent rounded-full" />
            )}
          </NavLink>
        ))}
      </nav>

      <aside className="hidden md:block p-4 space-y-4 sticky top-0 h-screen overflow-y-auto">
        <div className="bg-surface rounded-2xl p-4 border border-border">
          <h3 className="font-bold mb-3">Trending vibes</h3>
          {trending.length === 0 && <p className="text-xs text-muted">No posts yet.</p>}
          {trending.map((t) => (
            <div key={t.vibe} className="py-2 text-sm">
              <p className="text-muted text-xs">{t.count} posts</p>
              <p className="font-medium">{t.vibe}</p>
            </div>
          ))}
        </div>
        <div className="bg-surface rounded-2xl p-4 border border-border">
          <h3 className="font-bold mb-3">Who to follow</h3>
          {suggestions.length === 0 && <p className="text-xs text-muted">No suggestions right now.</p>}
          {!isGuest && suggestions.map((s) => (
            <div key={s.id} className="flex items-center justify-between py-2 text-sm">
              <Link to={`/profile/${s.handle}`} className="hover:underline">{s.name} · @{s.handle}</Link>
              <button onClick={() => follow(s.handle, s.id)} className="text-xs bg-text text-ink rounded-full px-3 py-1 font-semibold">
                Follow
              </button>
            </div>
          ))}
        </div>
      </aside>
    </div>
  );
}
