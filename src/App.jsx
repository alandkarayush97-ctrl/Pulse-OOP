import { lazy, Suspense } from "react";
import { Routes, Route, Navigate } from "react-router-dom";
import { useAuthStore } from "./store/authStore";
import AppLayout from "./components/AppLayout.jsx";

import Login from "./pages/Login.jsx";
import Signup from "./pages/Signup.jsx";
import Home from "./pages/Home.jsx";
const Explore = lazy(() => import("./pages/Explore.jsx"));
const Notifications = lazy(() => import("./pages/Notifications.jsx"));
const Messages = lazy(() => import("./pages/Messages.jsx"));
const Profile = lazy(() => import("./pages/Profile.jsx"));
const Thread = lazy(() => import("./pages/Thread.jsx"));
const FollowList = lazy(() => import("./pages/FollowList.jsx"));
const NotFound = lazy(() => import("./pages/NotFound.jsx"));

function Protected({ children }) {
  const isAuthenticated = useAuthStore((s) => s.isAuthenticated);
  return isAuthenticated ? children : <Navigate to="/login" replace />;
}

export default function App() {
  return (
    <Suspense fallback={<div className="p-6 text-muted">Loading…</div>}>
    <Routes>
      <Route path="/login" element={<Login />} />
      <Route path="/signup" element={<Signup />} />

      <Route
        path="/"
        element={
          <Protected>
            <AppLayout />
          </Protected>
        }
      >
        <Route index element={<Home />} />
        <Route path="explore" element={<Explore />} />
        <Route path="notifications" element={<Notifications />} />
        <Route path="messages" element={<Messages />} />
        <Route path="profile/:handle" element={<Profile />} />
        <Route path="tweet/:id" element={<Thread />} />
        <Route path="profile/:handle/following" element={<FollowList mode="following" />} />
        <Route path="profile/:handle/followers" element={<FollowList mode="followers" />} />
      </Route>

      <Route path="*" element={<NotFound />} />
    </Routes>
    </Suspense>
  );
}
