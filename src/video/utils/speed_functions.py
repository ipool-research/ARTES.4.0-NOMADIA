import matplotlib.pyplot as plt
import pandas as pd
import math
import cv2
from scipy.interpolate import interp1d


def get_fps_video(video_path):
    cap = cv2.VideoCapture(video_path)
    fps = cap.get(cv2.CAP_PROP_FPS)
    print(f"Frames Per Second (FPS): {fps}")    
    cap.release()
    return fps

def resample_speed(t1,s1,t2):
    speed_interp = interp1d(t1, s1, kind='linear', fill_value="extrapolate")
    s2 = speed_interp(t2)    
    return s2

def plot_resample_speeds(t1,s1,fs1,t2,s2,fs2):
    # Create subplots
    fig, axs = plt.subplots(2, 1, figsize=(10, 6))

    # Plot original speed (at ~18 Hz)
    axs[0].plot(t1, s1, label='Original Speed (~'+ str(round(fs1)) +' Hz)', color='b')
    axs[0].set_title('Original Speed at ~'+ str(round(fs1)) +' Hz')
    axs[0].set_xlabel('Time (s)')
    axs[0].set_ylabel('Speed (m/s)')
    axs[0].legend()

    # Plot resampled speed (at FPS Hz)
    axs[1].plot(t2, s2, label='Resampled Speed (~'+ str(math.ceil(fs2)) +' Hz)', color='r')
    axs[1].set_title('Resampled Speed at ~' + str(math.ceil(fs2)) + 'Hz')
    axs[1].set_xlabel('Time (s)')
    axs[1].set_ylabel('Speed (m/s)')
    axs[1].legend()

    # Adjust layout and display the plot
    plt.tight_layout()
    plt.show()