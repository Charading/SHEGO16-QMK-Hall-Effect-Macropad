#!/usr/bin/env python3
"""
GUI GIF to Keyboard Animation Uploader
"""

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import hid
import time
import threading
from PIL import Image, ImageTk
import os

# Your keyboard's VID/PID
VID = 0xFADE
PID = 0x0666

# Animation protocol constants
ANIM_CMD_MAGIC = 0xA5
ANIM_CMD_UPLOAD_START = 0x10
ANIM_CMD_UPLOAD_DATA = 0x11
ANIM_CMD_UPLOAD_END = 0x12
ANIM_CMD_PLAY_ANIMATION = 0x13
ANIM_CMD_STOP_ANIMATION = 0x16

class GIFUploaderGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Keyboard GIF Uploader")
        self.root.geometry("600x700")
        
        self.device = None
        self.current_gif = None
        self.preview_frames = []
        
        self.setup_ui()
        self.check_keyboard_connection()
        
    def setup_ui(self):
        # Connection status
        self.status_frame = tk.Frame(self.root)
        self.status_frame.pack(fill="x", padx=10, pady=5)
        
        self.status_label = tk.Label(self.status_frame, text="Checking keyboard...", 
                                   fg="orange", font=("Arial", 10, "bold"))
        self.status_label.pack(side="left")
        
        self.refresh_btn = tk.Button(self.status_frame, text="Refresh", 
                                   command=self.check_keyboard_connection)
        self.refresh_btn.pack(side="right")
        
        # VID/PID settings
        vid_pid_frame = tk.Frame(self.root)
        vid_pid_frame.pack(fill="x", padx=10, pady=5)
        
        tk.Label(vid_pid_frame, text="VID:").pack(side="left")
        self.vid_entry = tk.Entry(vid_pid_frame, width=8)
        self.vid_entry.insert(0, f"{VID:04X}")
        self.vid_entry.pack(side="left", padx=(5,10))
        
        tk.Label(vid_pid_frame, text="PID:").pack(side="left")
        self.pid_entry = tk.Entry(vid_pid_frame, width=8)
        self.pid_entry.insert(0, f"{PID:04X}")
        self.pid_entry.pack(side="left", padx=5)
        
        # File selection
        file_frame = tk.Frame(self.root)
        file_frame.pack(fill="x", padx=10, pady=10)
        
        tk.Label(file_frame, text="GIF File:", font=("Arial", 10, "bold")).pack(anchor="w")
        
        file_select_frame = tk.Frame(file_frame)
        file_select_frame.pack(fill="x", pady=5)
        
        self.file_entry = tk.Entry(file_select_frame)
        self.file_entry.pack(side="left", fill="x", expand=True)
        
        browse_btn = tk.Button(file_select_frame, text="Browse", command=self.browse_file)
        browse_btn.pack(side="right", padx=(5,0))
        
        # Preview
        preview_frame = tk.Frame(self.root)
        preview_frame.pack(fill="both", expand=True, padx=10, pady=10)
        
        tk.Label(preview_frame, text="Preview:", font=("Arial", 10, "bold")).pack(anchor="w")
        
        self.preview_canvas = tk.Canvas(preview_frame, width=200, height=200, bg="black")
        self.preview_canvas.pack(pady=5)
        
        # Settings
        settings_frame = tk.LabelFrame(self.root, text="Settings", font=("Arial", 10, "bold"))
        settings_frame.pack(fill="x", padx=10, pady=10)
        
        # Animation ID
        id_frame = tk.Frame(settings_frame)
        id_frame.pack(fill="x", padx=5, pady=5)
        tk.Label(id_frame, text="Animation ID (0-4):").pack(side="left")
        self.id_spinbox = tk.Spinbox(id_frame, from_=0, to=4, width=5, value=0)
        self.id_spinbox.pack(side="right")
        
        # Dimensions
        dim_frame = tk.Frame(settings_frame)
        dim_frame.pack(fill="x", padx=5, pady=5)
        tk.Label(dim_frame, text="Size:").pack(side="left")
        
        size_right_frame = tk.Frame(dim_frame)
        size_right_frame.pack(side="right")
        
        self.width_spinbox = tk.Spinbox(size_right_frame, from_=8, to=64, width=5, value=32)
        self.width_spinbox.pack(side="left")
        tk.Label(size_right_frame, text="x").pack(side="left")
        self.height_spinbox = tk.Spinbox(size_right_frame, from_=8, to=64, width=5, value=32)
        self.height_spinbox.pack(side="left")
        
        # Threshold
        thresh_frame = tk.Frame(settings_frame)
        thresh_frame.pack(fill="x", padx=5, pady=5)
        tk.Label(thresh_frame, text="B&W Threshold (0-255):").pack(side="left")
        self.threshold_spinbox = tk.Spinbox(thresh_frame, from_=0, to=255, width=8, value=128)
        self.threshold_spinbox.pack(side="right")
        
        # Progress
        self.progress_frame = tk.Frame(self.root)
        self.progress_frame.pack(fill="x", padx=10, pady=5)
        
        self.progress_label = tk.Label(self.progress_frame, text="Ready")
        self.progress_label.pack(anchor="w")
        
        self.progress_bar = ttk.Progressbar(self.progress_frame, mode="determinate")
        self.progress_bar.pack(fill="x", pady=5)
        
        # Buttons
        button_frame = tk.Frame(self.root)
        button_frame.pack(fill="x", padx=10, pady=10)
        
        self.upload_btn = tk.Button(button_frame, text="Upload", command=self.upload_animation,
                                  bg="blue", fg="white", font=("Arial", 10, "bold"))
        self.upload_btn.pack(side="left", padx=(0,5))
        
        self.play_btn = tk.Button(button_frame, text="Play", command=self.play_animation,
                                bg="green", fg="white", font=("Arial", 10, "bold"))
        self.play_btn.pack(side="left", padx=5)
        
        self.stop_btn = tk.Button(button_frame, text="Stop", command=self.stop_animation,
                                bg="red", fg="white", font=("Arial", 10, "bold"))
        self.stop_btn.pack(side="left", padx=5)
        
        self.upload_play_btn = tk.Button(button_frame, text="Upload & Play", 
                                       command=self.upload_and_play,
                                       bg="purple", fg="white", font=("Arial", 10, "bold"))
        self.upload_play_btn.pack(side="right")
        
    def check_keyboard_connection(self):
        try:
            # Update VID/PID from entries
            global VID, PID
            VID = int(self.vid_entry.get(), 16)
            PID = int(self.pid_entry.get(), 16)
            
            if self.device:
                self.device.close()
                
            self.device = hid.device()
            self.device.open(VID, PID)
            self.status_label.config(text=f"✓ Keyboard connected (VID: 0x{VID:04X}, PID: 0x{PID:04X})", 
                                   fg="green")
            self.enable_buttons(True)
        except Exception as e:
            self.status_label.config(text=f"✗ Keyboard not found: {str(e)}", fg="red")
            self.device = None
            self.enable_buttons(False)
            
    def enable_buttons(self, enabled):
        state = "normal" if enabled else "disabled"
        self.upload_btn.config(state=state)
        self.play_btn.config(state=state)
        self.stop_btn.config(state=state)
        self.upload_play_btn.config(state=state)
        
    def browse_file(self):
        filename = filedialog.askopenfilename(
            title="Select GIF file",
            filetypes=[("GIF files", "*.gif"), ("All files", "*.*")]
        )
        if filename:
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, filename)
            self.load_preview(filename)
            
    def load_preview(self, filename):
        try:
            with Image.open(filename) as img:
                # Create preview frames
                self.preview_frames = []
                for frame_num in range(min(img.n_frames, 10)):  # Limit to 10 frames for preview
                    img.seek(frame_num)
                    
                    # Resize for preview
                    preview_size = (150, 150)
                    frame = img.convert('RGB').resize(preview_size, Image.Resampling.LANCZOS)
                    
                    # Convert to PhotoImage
                    photo = ImageTk.PhotoImage(frame)
                    self.preview_frames.append(photo)
                
                # Show first frame
                if self.preview_frames:
                    self.preview_canvas.delete("all")
                    self.preview_canvas.create_image(100, 100, image=self.preview_frames[0])
                    
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load preview: {str(e)}")
            
    def convert_gif_to_frames(self, gif_path, target_width, target_height, threshold):
        """Convert GIF to 1-bit bitmap frames"""
        frames = []
        frame_delays = []
        
        with Image.open(gif_path) as img:
            for frame_num in range(img.n_frames):
                img.seek(frame_num)
                
                # Get frame duration (in ms)
                duration = img.info.get('duration', 100)
                frame_delays.append(duration)
                
                # Resize and convert to grayscale
                frame = img.convert('RGB').resize((target_width, target_height), Image.Resampling.LANCZOS)
                frame = frame.convert('L')
                
                # Convert to 1-bit (binary)
                frame = frame.point(lambda x: 255 if x > threshold else 0, '1')
                
                # Convert to bytes (1 bit per pixel, packed)
                frame_bytes = []
                pixels = list(frame.getdata())
                
                for i in range(0, len(pixels), 8):
                    byte = 0
                    for j in range(8):
                        if i + j < len(pixels) and pixels[i + j]:
                            byte |= (1 << (7 - j))
                    frame_bytes.append(byte)
                
                frames.append(bytes(frame_bytes))
                
            # Use average frame delay
            avg_delay = sum(frame_delays) // len(frame_delays) if frame_delays else 100
            
        return frames, avg_delay
        
    def upload_animation_worker(self, gif_path, animation_id, target_width, target_height, threshold):
        try:
            self.root.after(0, lambda: self.progress_label.config(text="Converting GIF..."))
            
            # Convert GIF
            frames, frame_delay = self.convert_gif_to_frames(gif_path, target_width, target_height, threshold)
            
            if not frames:
                self.root.after(0, lambda: messagebox.showerror("Error", "No frames extracted from GIF"))
                return False
                
            # Generate name from filename
            name = os.path.splitext(os.path.basename(gif_path))[0][:15]
            
            self.root.after(0, lambda: self.progress_label.config(text=f"Uploading {len(frames)} frames..."))
            
            # Upload animation
            success = self.upload_frames(animation_id, frames, target_width, target_height, frame_delay, name)
            
            if success:
                self.root.after(0, lambda: self.progress_label.config(text="Upload complete!"))
                self.root.after(0, lambda: messagebox.showinfo("Success", f"Animation '{name}' uploaded successfully!"))
            else:
                self.root.after(0, lambda: self.progress_label.config(text="Upload failed"))
                self.root.after(0, lambda: messagebox.showerror("Error", "Upload failed"))
                
            return success
            
        except Exception as e:
            self.root.after(0, lambda: messagebox.showerror("Error", f"Upload error: {str(e)}"))
            return False
        finally:
            self.root.after(0, lambda: self.progress_bar.config(value=0))
            self.root.after(0, lambda: self.enable_buttons(True))
            
    def upload_frames(self, animation_id, frames, width, height, frame_delay, name):
        if not self.device:
            return False
            
        # Send start command
        packet = bytearray(32)
        packet[0] = ANIM_CMD_MAGIC
        packet[1] = ANIM_CMD_UPLOAD_START
        packet[2] = animation_id
        packet[3] = 24
        packet[4] = width
        packet[5] = height
        packet[6] = len(frames)
        packet[7] = frame_delay & 0xFF
        packet[8] = (frame_delay >> 8) & 0xFF
        
        name_bytes = name.encode('utf-8')[:16]
        packet[9:9+len(name_bytes)] = name_bytes
        
        try:
            self.device.write(packet)
            time.sleep(0.01)
        except Exception as e:
            return False
            
        # Upload frame data
        packet_index = 0
        total_bytes = sum(len(frame) for frame in frames)
        uploaded_bytes = 0
        
        for frame_idx, frame in enumerate(frames):
            for i in range(0, len(frame), 26):
                chunk = frame[i:i+26]
                
                packet = bytearray(32)
                packet[0] = ANIM_CMD_MAGIC
                packet[1] = ANIM_CMD_UPLOAD_DATA
                packet[2] = animation_id
                packet[3] = len(chunk)
                packet[4] = packet_index & 0xFF
                packet[5] = (packet_index >> 8) & 0xFF
                packet[6:6+len(chunk)] = chunk
                
                try:
                    self.device.write(packet)
                    uploaded_bytes += len(chunk)
                    packet_index += 1
                    
                    # Update progress
                    progress = (uploaded_bytes * 100) // total_bytes
                    self.root.after(0, lambda p=progress: self.progress_bar.config(value=p))
                    
                    time.sleep(0.005)
                    
                except Exception as e:
                    return False
                    
        # Send end command
        packet = bytearray(32)
        packet[0] = ANIM_CMD_MAGIC
        packet[1] = ANIM_CMD_UPLOAD_END
        packet[2] = animation_id
        
        try:
            self.device.write(packet)
            return True
        except Exception as e:
            return False
            
    def upload_animation(self):
        if not self.device:
            messagebox.showerror("Error", "Keyboard not connected")
            return
            
        gif_path = self.file_entry.get()
        if not gif_path or not os.path.exists(gif_path):
            messagebox.showerror("Error", "Please select a valid GIF file")
            return
            
        try:
            animation_id = int(self.id_spinbox.get())
            target_width = int(self.width_spinbox.get())
            target_height = int(self.height_spinbox.get())
            threshold = int(self.threshold_spinbox.get())
        except ValueError:
            messagebox.showerror("Error", "Invalid settings")
            return
            
        self.enable_buttons(False)
        self.progress_bar.config(value=0)
        
        # Run upload in separate thread
        thread = threading.Thread(target=self.upload_animation_worker, 
                                args=(gif_path, animation_id, target_width, target_height, threshold))
        thread.daemon = True
        thread.start()
        
    def play_animation(self):
        if not self.device:
            messagebox.showerror("Error", "Keyboard not connected")
            return
            
        try:
            animation_id = int(self.id_spinbox.get())
            
            packet = bytearray(32)
            packet[0] = ANIM_CMD_MAGIC
            packet[1] = ANIM_CMD_PLAY_ANIMATION
            packet[2] = animation_id
            
            self.device.write(packet)
            self.progress_label.config(text=f"Playing animation {animation_id}")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to play animation: {str(e)}")
            
    def stop_animation(self):
        if not self.device:
            messagebox.showerror("Error", "Keyboard not connected")
            return
            
        try:
            packet = bytearray(32)
            packet[0] = ANIM_CMD_MAGIC
            packet[1] = ANIM_CMD_STOP_ANIMATION
            
            self.device.write(packet)
            self.progress_label.config(text="Animation stopped")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to stop animation: {str(e)}")
            
    def upload_and_play(self):
        def play_after_upload():
            # Wait a bit for upload to complete, then play
            time.sleep(1)
            self.root.after(0, self.play_animation)
            
        # Start upload
        self.upload_animation()
        
        # Schedule play command
        thread = threading.Thread(target=play_after_upload)
        thread.daemon = True
        thread.start()
        
    def __del__(self):
        if self.device:
            self.device.close()

def main():
    root = tk.Tk()
    app = GIFUploaderGUI(root)
    
    def on_closing():
        if app.device:
            app.device.close()
        root.destroy()
    
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()

if __name__ == '__main__':
    main()
