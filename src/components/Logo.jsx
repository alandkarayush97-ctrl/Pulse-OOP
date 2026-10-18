export default function Logo({ size = 26 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path
        d="M16 3 C22 3 27 7.5 27 13.5 C27 19.5 22 24 16 24 C13.8 24 11.8 23.4 10.1 22.3 L5 24 L6.7 19.2 C5.2 17.6 4.3 15.6 4.3 13.5 C4.3 7.5 10 3 16 3 Z"
        fill="#6C5CE7"
      />
      <circle cx="12" cy="13.5" r="1.6" fill="#0F1113" />
      <circle cx="16" cy="13.5" r="1.6" fill="#0F1113" />
      <circle cx="20" cy="13.5" r="1.6" fill="#0F1113" />
    </svg>
  );
}
