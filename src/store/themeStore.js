import { create } from "zustand";

const apply = (isLight) => {
  document.documentElement.classList.toggle("light", isLight);
  localStorage.setItem("theme", isLight ? "light" : "dark");
};

const saved = typeof window !== "undefined" ? localStorage.getItem("theme") : "dark";
apply(saved === "light");

export const useThemeStore = create((set, get) => ({
  isLight: saved === "light",
  toggleTheme: () => {
    const next = !get().isLight;
    apply(next);
    set({ isLight: next });
  },
}));
