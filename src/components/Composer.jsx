import { useState, useRef } from "react";
import { Image, Smile, MapPin, Clock, Users, X } from "lucide-react";
import { useAuthStore } from "../store/authStore";

const ENERGY_TAGS = ["Just sharing", "Venting", "Celebrating", "Asking advice"];
const MAX_FILE_MB = 8;

export default function Composer({ onPost }) {
  const [text, setText] = useState("");
  const [energy, setEnergy] = useState("Just sharing");
  const [scheduling, setScheduling] = useState(false);
  const [publishAt, setPublishAt] = useState("");
  const [showCoauthor, setShowCoauthor] = useState(false);
  const [coAuthorHandle, setCoAuthorHandle] = useState("");
  const [mediaUrl, setMediaUrl] = useState(null);
  const [mediaType, setMediaType] = useState(null); // "image" | "video" | "audio"
  const fileRef = useRef(null);
  const { user } = useAuthStore();

  const handleFile = (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    if (file.size > MAX_FILE_MB * 1024 * 1024) {
      alert(`File too large (max ${MAX_FILE_MB}MB)`);
      return;
    }
    const type = file.type.startsWith("video") ? "video" : file.type.startsWith("audio") ? "audio" : "image";
    const reader = new FileReader();
    reader.onload = () => {
      setMediaUrl(reader.result);
      setMediaType(type);
    };
    reader.readAsDataURL(file);
  };

  const clearMedia = () => {
    setMediaUrl(null);
    setMediaType(null);
    if (fileRef.current) fileRef.current.value = "";
  };

  const submit = () => {
    if (!text.trim()) return;
    onPost?.({
      content: text.trim(),
      energy,
      publishAt: scheduling && publishAt ? publishAt : null,
      coAuthorHandle: showCoauthor && coAuthorHandle.trim() ? coAuthorHandle.trim() : null,
      imageUrl: mediaUrl,
    });
    setText("");
    setScheduling(false);
    setPublishAt("");
    setShowCoauthor(false);
    setCoAuthorHandle("");
    clearMedia();
  };

  return (
    <div className="border-b border-border p-4 flex gap-3">
      <div className="w-11 h-11 rounded-full bg-accent/20 flex items-center justify-center font-bold text-accent shrink-0">
        {user?.name?.[0] ?? "U"}
      </div>
      <div className="flex-1">
        <textarea
          value={text}
          onChange={(e) => setText(e.target.value)}
          placeholder="What's happening?"
          rows={2}
          className="w-full bg-transparent resize-none outline-none text-lg placeholder:text-muted"
        />

        {mediaUrl && (
          <div className="relative mb-2 inline-block">
            {mediaType === "image" && <img src={mediaUrl} alt="attachment preview" className="max-h-48 rounded-lg border border-border" />}
            {mediaType === "video" && <video src={mediaUrl} controls className="max-h-48 rounded-lg border border-border" />}
            {mediaType === "audio" && <audio src={mediaUrl} controls className="w-64" />}
            <button
              onClick={clearMedia}
              className="absolute top-1 right-1 bg-ink/80 rounded-full p-1"
            >
              <X size={14} />
            </button>
          </div>
        )}

        <div className="flex gap-1.5 flex-wrap mb-2">
          {ENERGY_TAGS.map((tag) => (
            <button
              key={tag}
              onClick={() => setEnergy(tag)}
              className={`text-xs px-2.5 py-1 rounded-full border ${
                energy === tag ? "bg-accent border-accent text-white" : "border-border text-muted hover:text-text"
              }`}
            >
              {tag}
            </button>
          ))}
        </div>

        {scheduling && (
          <input
            type="datetime-local"
            value={publishAt}
            onChange={(e) => setPublishAt(e.target.value)}
            className="bg-surface border border-border rounded-lg px-2 py-1 text-sm mb-2 outline-none focus:border-accent"
          />
        )}

        {showCoauthor && (
          <input
            value={coAuthorHandle}
            onChange={(e) => setCoAuthorHandle(e.target.value)}
            placeholder="co-author's @handle"
            className="bg-surface border border-border rounded-lg px-2 py-1 text-sm mb-2 outline-none focus:border-accent w-full"
          />
        )}

        <div className="flex items-center justify-between mt-2">
          <div className="flex gap-3 text-accent">
            <input ref={fileRef} type="file" accept="image/*,video/*,audio/*" onChange={handleFile} className="hidden" />
            <Image size={18} className="cursor-pointer" onClick={() => fileRef.current?.click()} title="Add image, video, or audio" />
            <Smile size={18} className="cursor-pointer" />
            <MapPin size={18} className="cursor-pointer" />
            <button
              onClick={() => setScheduling(!scheduling)}
              title="Time Capsule: schedule for later"
              className={scheduling ? "text-accent" : "text-muted hover:text-accent"}
            >
              <Clock size={18} />
            </button>
            <button
              onClick={() => setShowCoauthor(!showCoauthor)}
              title="Co-author this post"
              className={showCoauthor ? "text-accent" : "text-muted hover:text-accent"}
            >
              <Users size={18} />
            </button>
          </div>
          <button
            onClick={submit}
            disabled={!text.trim()}
            className="bg-accent disabled:opacity-40 hover:bg-accentDim text-white rounded-full px-5 py-1.5 font-semibold text-sm"
          >
            Post
          </button>
        </div>
      </div>
    </div>
  );
}
