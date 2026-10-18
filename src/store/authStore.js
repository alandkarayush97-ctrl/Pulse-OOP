import { create } from "zustand";
import { api } from "../services/api";

export const useAuthStore = create((set, get) => ({
  user: JSON.parse(localStorage.getItem("user") || "null"),
  isAuthenticated: !!localStorage.getItem("token"),

  login: async (email, password) => {
    const { token, user } = await api.login({ email, password });
    localStorage.setItem("token", token);
    localStorage.setItem("user", JSON.stringify(user));
    set({ user, isAuthenticated: true });
  },

  guestLogin: async () => {
    const { token, user } = await api.guestLogin();
    localStorage.setItem("token", token);
    localStorage.setItem("user", JSON.stringify(user));
    set({ user, isAuthenticated: true });
  },

  signup: async ({ name, email, password }) => {
    const { token, user } = await api.signup({ name, email, password });
    localStorage.setItem("token", token);
    localStorage.setItem("user", JSON.stringify(user));
    set({ user, isAuthenticated: true });
  },

  setAvatar: (avatarUrl) => {
    const updated = { ...get().user, avatarUrl };
    localStorage.setItem("user", JSON.stringify(updated));
    set({ user: updated });
  },

  logout: () => {
    localStorage.removeItem("token");
    localStorage.removeItem("user");
    set({ user: null, isAuthenticated: false });
  },
}));
