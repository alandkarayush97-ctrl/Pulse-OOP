import { Link } from "react-router-dom";

export default function NotFound() {
  return (
    <div className="min-h-screen flex flex-col items-center justify-center bg-ink text-center px-4">
      <h1 className="text-6xl font-bold text-accent">404</h1>
      <p className="text-muted mt-2">This page doesn't exist.</p>
      <Link to="/" className="mt-6 bg-accent hover:bg-accentDim rounded-full px-5 py-2 font-semibold">
        Back to Home
      </Link>
    </div>
  );
}
