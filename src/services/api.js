const BASE_URL = "http://127.0.0.1:4000/api";

async function request(path, options = {}) {
  const token = localStorage.getItem("token");
  const res = await fetch(`${BASE_URL}${path}`, {
    method: options.method || "GET",
    headers: {
      "Content-Type": "application/json",
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
    },
    body: options.body ? JSON.stringify(options.body) : undefined,
  });

  if (res.status === 401) {
    localStorage.removeItem("token");
    localStorage.removeItem("user");
    window.location.href = "/login";
    throw new Error("Session expired");
  }

  const data = await res.json().catch(() => ({}));
  if (res.status === 403 && data.error) { window.alert(data.error); throw new Error(data.error); }
  if (!res.ok) throw new Error(data.error || "Request failed");
  return data;
}

export const api = {
  signup: (payload) => request("/auth/signup", { method: "POST", body: payload }),
  forgotPassword: (email) => request("/auth/forgot", { method: "POST", body: { email } }),
  resetPassword: (payload) => request("/auth/reset", { method: "POST", body: payload }),
  guestLogin: () => request("/auth/guest", { method: "POST" }),
  login: (payload) => request("/auth/login", { method: "POST", body: payload }),

  getFeed: (vibe, { limit = 20, offset = 0 } = {}) =>
    request(`/tweets?limit=${limit}&offset=${offset}${vibe && vibe !== "All" ? `&vibe=${encodeURIComponent(vibe)}` : ""}`),
  getLatestTweetId: () => request("/tweets/latest-id"),
  searchTweets: (q) => request(`/tweets/search?q=${encodeURIComponent(q)}`),
  createTweet: (payload) => request("/tweets", { method: "POST", body: payload }),
  updateTweet: (id, content) => request(`/tweets/${id}`, { method: "PATCH", body: { content } }),
  deleteTweet: (id) => request(`/tweets/${id}`, { method: "DELETE" }),
  likeTweet: (id) => request(`/tweets/${id}/like`, { method: "POST" }),
  retweetTweet: (id) => request(`/tweets/${id}/retweet`, { method: "POST" }),
  reactTweet: (id, emoji) => request(`/tweets/${id}/react`, { method: "POST", body: { emoji } }),
  factCheckTweet: (id) => request(`/tweets/${id}/factcheck`, { method: "POST" }),
  respondCoauthor: (id, accept) => request(`/tweets/${id}/coauthor/respond`, { method: "POST", body: { accept } }),
  getTrending: () => request("/tweets/trending"),

  getProfile: (handle) => request(`/users/${handle}`),
  getUserTweets: (handle) => request(`/users/${handle}/tweets`),
  toggleFollow: (handle) => request(`/users/${handle}/follow`, { method: "POST" }),
  getSuggestions: () => request("/users/suggestions"),
  getFollowing: (handle) => request(`/users/${handle}/following`),
  getFollowers: (handle) => request(`/users/${handle}/followers`),
  updateAvatar: (avatarUrl) => request("/users/me/avatar", { method: "POST", body: { avatarUrl } }),

  getNotifications: () => request("/notifications"),
  markNotificationsRead: () => request("/notifications/read", { method: "POST" }),

  searchUsers: (q) => request(`/users/search?q=${encodeURIComponent(q)}`),

  getConversations: () => request("/messages"),
  getThread: (userId) => request(`/messages/${userId}`),
  sendMessage: (userId, content) => request(`/messages/${userId}`, { method: "POST", body: { content } }),
  getUnreadMessageCount: () => request("/messages/unread-count"),

  getTweet: (id) => request(`/tweets/${id}`),
  getReplies: (id) => request(`/tweets/${id}/replies`),
  createReply: (id, payload) => request("/tweets", { method: "POST", body: { ...payload, replyTo: id } }),
};
