import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import yt_dlp
from yt_dlp.utils import DownloadError
import threading

#initialize window
root = tk.Tk()

#initialize window settings
root.title("Download youtube playlists/vids!")
root.minsize(520, 320)
root.maxsize(1920, 1080)
root.geometry("800x500")


#frames
main = ttk.Frame(root, padding=20)
main.pack(fill="both", expand=True)

header = ttk.Frame(main)
header.pack(fill="x", pady=(0, 15))

content = ttk.Frame(main)
content.pack(fill="x")


footer = ttk.Frame(main)
footer.pack(fill="x", pady=(15, 0))

#display text
title = ttk.Label(
    header, 
    text="This is a simple youtube/playlist downloader", 
    )
title.pack(anchor="w")

subtitle = ttk.Label(
    root, 
    text="Just put the Url inside the box that says paste your Url and let it do its thing!", 
    )
subtitle.pack(anchor="w")

#execute code
def download():
    print("DOWNLOAD THREAD STARTED")
    playlist_url = entry.get()
    
    try:
        ydl_opts = {
            "format": "bestvideo+bestaudio/best",
            "merge_output_format": "mp4",
            "progress_hooks": [progress_hook],
            "outtmpl": "%(playlist_title)s/%(title)s.%(ext)s",
            "quiet": True
        }

        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            result = ydl.download([playlist_url])
        if result == 0:
            root.after(0, update_progress, 100, "download finished")
        else:
            root.after(0, update_progress, 0, f"download failed")
    except DownloadError as e:
        msg = str(e)
        
        root.after(0, update_progress, 0, f"Download failed")
        messagebox.showerror(title="Ding Dong, something went wrong", message=msg)
    except Exception as e:
        root.after(0, update_progress, 0)
        messagebox.showerror(title="UwU", message=str(e))
    
    

def start_download():
    update_progress(0, "Starting download...")

    thread = threading.Thread(target=download, daemon=True)
    thread.start()

def update_progress(value, text=None):
    progress['value'] = value
    if text:
        status_label.config(text=text)
def progress_hook(d):
    if d["status"] == "downloading":
        percent = d.get("_percent")

        if percent is not None:
            root.after(
                0,
                update_progress,
                percent,
                f"downloading... {percent:.1f}%"
            )


#initialize entrys
entry = ttk.Entry(root)
entry.insert(0, "")
entry.pack(padx=5, pady=10, fill="x")
entry.focus()

#initialize progressbars
status_label = ttk.Label(
    footer, 
    text="idle", 
    )
status_label.pack(anchor="w")

progress = ttk.Progressbar(
    footer, 
    mode="determinate", 
    maximum=100
    )
progress.pack(fill="x", pady=(0, 8))

#initialize buttons
download_button = ttk.Button(
    content,
    text="start download",
    command=start_download,
).pack(fill="x")

exit_button = ttk.Button(
    root, 
    text="Click to exit program",
    command=lambda: root.quit()
).pack(fill="x")

root.mainloop()