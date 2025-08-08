import matplotlib.pyplot as plt
import os
import cv2

def calculate_frame_extraction_points(speed_data, fps, visual_coverage):
    # Initialize cumulative displacement and time variables
    cumulative_displacement = 0
    extraction_points = [0]  # To store frame indices where a frame should be extracted
    frame_time = 0
    frame_interval = 1 / fps  # Time between frames in seconds
    flag = 0
    # Loop through speed data to calculate displacement
    for i, speed in enumerate(speed_data):
        if speed < 1.5:
            continue
        else:
            if flag == 0:
                extraction_points.append(i)
                flag = 1 
            displacement = speed * frame_interval  # Displacement for the current frame

            # Accumulate the displacement
            cumulative_displacement += displacement

            # If displacement reaches the desired threshold (visual_coverage - overlap)
            if cumulative_displacement >= (visual_coverage):
                extraction_points.append(i)  # Record the frame index
                cumulative_displacement = 0  # Reset the displacement for the next segment

            # Increment time
            frame_time += frame_interval

    return extraction_points

def extract_specific_frames(video_path, extraction_points, fps, output_folder):
    # Create the output directory if it does not exist
    os.makedirs(output_folder, exist_ok=True)

    # Open the video file
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return

    for i, frame_idx in enumerate(extraction_points):
        # Set the video position to the specific frame index
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        
        # Read the frame
        ret, frame = cap.read()
        
        if ret:
            # Define the output frame filename
            output_frame = f'{output_folder}/frame_{i}.png'
            cv2.imwrite(output_frame, frame)  # Save the extracted frame
            print(f"Extracted frame {i} at index {frame_idx}")
        else:
            print(f"Warning: Could not read frame at index {frame_idx}")

    # Release the video capture object
    cap.release()


def extract_specific_frames2(video_path, extraction_points, fps, output_folder):

    # Create the output directory if it does not exist
    os.makedirs(output_folder, exist_ok=True)

    # Open the video file
    # Initialize the extraction from the first video in the folder
    videos = [f for f in os.listdir(video_path) if f.endswith('.MP4')] 
    print(videos)
    #sort the videos 
    
    videos = sorted(videos,key = lambda x: int(x.split('.MP4')[0][2:]))
    print(videos)

    index_video = 0
    cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
    

    if not cap.isOpened():
        print("Error: Could not open video.")
        return
        
    for i in range(len(extraction_points)):
        
        # Set the video position to the specific frame index
        frame_idx = extraction_points[i]
        print('Number of frames video:',int(cap.get(cv2.CAP_PROP_FRAME_COUNT)))
        print('Frame index:',frame_idx)     

        #check if the frame index is within the video space
        if frame_idx > int(cap.get(cv2.CAP_PROP_FRAME_COUNT)):
        
            index_video += 1

            if index_video == len(videos):
                print('Concluded the extraction of frames')
                break
            
            print('Jumped at video ',videos[index_video])

            extraction_points = extraction_points - int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            cap.release()

            cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
            
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        
        # Read the frame
        ret, frame = cap.read()
        
        if ret:
            # Define the output frame filename
            output_frame = f'{output_folder}/frame_{extraction_points[i]}.png'
            cv2.imwrite(output_frame, frame)  # Save the extracted frame
            print(f"Extracted frame {i} at index {frame_idx} of video {index_video}")

        else:

            print(f"Warning: Could not read frame at index {frame_idx}")

    # Release the video capture object
    cap.release()

def extract_specific_frames2_512(video_path, extraction_points, fps, output_folder, crop_size, height = 0):
    """
    Extract the frames when the image has been wrongly calibrated. Check the new points to define the reshape of the picture (square angle definition).
    input: height is the pixel height in the image where you want to cut it 
    """
    resize_size=(512, 512)
    # Create the output directory if it does not exist
    os.makedirs(output_folder, exist_ok=True)

    # Open the video file
    # Initialize the extraction from the first video in the folder
    videos = [f for f in os.listdir(video_path) if f.endswith('.MP4')] 
    print(videos)
    #sort the videos 
    
    videos = sorted(videos,key = lambda x: int(x.split('.MP4')[0][2:]))
    print(videos)

    index_video = 0
    cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return
    
    
    for i in range(len(extraction_points)):
        
        # Set the video position to the specific frame index
        frame_idx = extraction_points[i]
        print('Number of frames video:',int(cap.get(cv2.CAP_PROP_FRAME_COUNT)))
        print('Frame index:',frame_idx)     

        #check if the frame index is within the video space
        if frame_idx > int(cap.get(cv2.CAP_PROP_FRAME_COUNT)):
        
            index_video += 1

            if index_video == len(videos):
                print('Concluded the extraction of frames')
                break
            
            print('Jumped at video ',videos[index_video])

            extraction_points = extraction_points - int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            cap.release()

            cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
            
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        
        # Read the frame
        ret, frame = cap.read()
        
        if ret:
            # change the y_start to cut the image at the defined height
            if height == 0:
                y_start = (frame.shape[0] - crop_size) // 2
            else: 
                # height is the pixel height calculating from the top of the image
                crop_size = frame.shape[0] - height
                y_start = height

            x_start = (frame.shape[1] - crop_size) // 2
            cropped_img = frame[y_start:y_start + crop_size, x_start:x_start + crop_size]

            # Ridimensiona a 512x512
            resized_img = cv2.resize(cropped_img, resize_size, interpolation=cv2.INTER_AREA)
            # Define the output frame filename
            car = os.path.basename(output_folder)
            output_frame = f'{output_folder}/{car}_frame_{frame_idx}.png'
            cv2.imwrite(output_frame, resized_img)  # Save the extracted frame
            #print(f"Extracted frame {i} at index {frame_idx}")
        else:
            print(f"Warning: Could not read frame at index {frame_idx}")

    # Release the video capture object
    cap.release()


def extract_specific_frames3_512_decentr(video_path, extraction_points, fps, output_folder, crop_size, height = 0, x_0 = 0):
    """
    Extract the frames when the image has been wrongly calibrated. Check the new points to define the reshape of the picture (square angle definition).
    input: height is the pixel height in the image where you want to cut it 
    Per Extra_07_14_2025 e per Urbane: height = 360, x_0 = 480
    Per Autostrada_07_15_2025: height = 340, x_0 = 480
    visual coverage = 5m
    """
    resize_size=(512, 512)
    # Create the output directory if it does not exist
    os.makedirs(output_folder, exist_ok=True)

    # Open the video file
    # Initialize the extraction from the first video in the folder
    videos = [f for f in os.listdir(video_path) if f.endswith('.MP4')] 
    print(videos)
    #sort the videos 
    
    videos = sorted(videos,key = lambda x: int(x.split('.MP4')[0][2:]))
    print(videos)

    index_video = 0
    cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return
    
    
    for i in range(len(extraction_points)):
        
        # Set the video position to the specific frame index
        frame_idx = extraction_points[i]
        print('Number of frames video:',int(cap.get(cv2.CAP_PROP_FRAME_COUNT)))
        print('Frame index:',frame_idx)     

        #check if the frame index is within the video space
        if frame_idx > int(cap.get(cv2.CAP_PROP_FRAME_COUNT)):
        
            index_video += 1

            if index_video == len(videos):
                print('Concluded the extraction of frames')
                break
            
            print('Jumped at video ',videos[index_video])

            extraction_points = extraction_points - int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            cap.release()

            cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
            
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        
        # Read the frame
        ret, frame = cap.read()
        
        if ret:
            # Coordinate per ritagliare il quadrato 1080x1080 al centro
            # change the y_start to cut the image at the defined height
            if height == 0:
                y_start = (frame.shape[0] - crop_size) // 2
            else: 
                crop_size = frame.shape[0] - height
                y_start = height
            if x_0 == 0:
                x_start = (frame.shape[1] - crop_size) // 2
            else:
                x_start = x_0
            
            cropped_img = frame[y_start:y_start + crop_size, x_start:x_start + crop_size]

            # Ridimensiona a 512x512
            resized_img = cv2.resize(cropped_img, resize_size, interpolation=cv2.INTER_AREA)
            # Define the output frame filename
            car = os.path.basename(output_folder)
            output_frame = f'{output_folder}/{car}_frame_{frame_idx}.png'
            cv2.imwrite(output_frame, resized_img)  # Save the extracted frame
            #print(f"Extracted frame {i} at index {frame_idx}")
        else:
            print(f"Warning: Could not read frame at index {frame_idx}")

    # Release the video capture object
    cap.release()


def extract_specific_frames4_512_two_squares(video_path, extraction_points, fps, output_folder, crop_size, height = 0, x_0 = 0):
    """
    Extract the frames when the image has been wrongly calibrated. Check the new points to define the reshape of the picture (square angle definition).
    input: height is the pixel height in the image where you want to cut it 
    Per Extra_07_14_2025 e per Urbane: height = 360, x_0 = 240
    Per Autostrada_07_15_2025: height = 340, x_0 = 240
    visual coverage = 5m
    """
    # Create the output directory if it does not exist
    os.makedirs(output_folder, exist_ok=True)

    output_folder_left = os.path.join(output_folder, 'left')
    output_folder_right = os.path.join(output_folder, 'right')
    if not os.path.exists(output_folder_left):
        os.makedirs(output_folder_left) 
    if not os.path.exists(output_folder_right):
        os.makedirs(output_folder_right)    

    resize_size=(512, 512)

    # Open the video file
    # Initialize the extraction from the first video in the folder
    videos = [f for f in os.listdir(video_path) if f.endswith('.MP4')] 
    print(videos)
    #sort the videos 
    
    videos = sorted(videos,key = lambda x: int(x.split('.MP4')[0][2:]))
    print(videos)

    index_video = 0
    cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
    
    if not cap.isOpened():
        print("Error: Could not open video.")
        return
    
    
    for i in range(len(extraction_points)):
        
        # Set the video position to the specific frame index
        frame_idx = extraction_points[i]
        print('Number of frames video:',int(cap.get(cv2.CAP_PROP_FRAME_COUNT)))
        print('Frame index:',frame_idx)     

        #check if the frame index is within the video space
        if frame_idx > int(cap.get(cv2.CAP_PROP_FRAME_COUNT)):
        
            index_video += 1

            if index_video == len(videos):
                print('Concluded the extraction of frames')
                break
            
            print('Jumped at video ',videos[index_video])

            extraction_points = extraction_points - int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            cap.release()

            cap = cv2.VideoCapture(os.path.join(video_path,videos[index_video]))
            
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
        
        # Read the frame
        ret, frame = cap.read()
        
        if ret:
            # Coordinate per ritagliare il quadrato 1080x1080 al centro
            # change the y_start to cut the image at the defined height
            if height == 0:
                y_start = (frame.shape[0] - crop_size) // 2
            else: 
                crop_size = frame.shape[0] - height
                y_start = height
            if x_0 == 0:
                x_start = (frame.shape[1] - crop_size) // 2
            else:
                x_start = x_0
                x_start_2 = x_start + crop_size
            
            cropped_img_1 = frame[y_start:y_start + crop_size, x_start:x_start + crop_size]
            cropped_img_2 = frame[y_start:y_start + crop_size, x_start_2:x_start_2 + crop_size]
            # Ridimensiona a 512x512
            resized_img_1 = cv2.resize(cropped_img_1, resize_size, interpolation=cv2.INTER_AREA)
            resized_img_2 = cv2.resize(cropped_img_2, resize_size, interpolation=cv2.INTER_AREA)
            # Define the output frame filename
            car = os.path.basename(output_folder)
            output_frame_1 = f'{output_folder_left}/{car}_l_frame_{frame_idx}.png'
            output_frame_2 = f'{output_folder_right}/{car}_r_frame_{frame_idx}.png'
            cv2.imwrite(output_frame_1, resized_img_1)  # Save the extracted frame
            cv2.imwrite(output_frame_2, resized_img_2)
            #print(f"Extracted frame {i} at index {frame_idx}")
        else:
            print(f"Warning: Could not read frame at index {frame_idx}")

    # Release the video capture object
    cap.release()




def plot_extraction_points(time,speed,extraction_points):
    # Plotting the speed data and extraction points
    plt.figure(figsize=(10, 6))
    plt.plot(time, speed, label='Speed (m/s)', color='blue')

    # Mark the extraction points on the graph
    for point in extraction_points:
        plt.axvline(time[point], color='red', linestyle='--', label=f'Frame Extracted at {time[point]:.2f}s')

    plt.title('Speed Data with Frame Extraction Points')
    plt.xlabel('Time (s)')
    plt.ylabel('Speed (m/s)')
    # plt.legend(loc='upper right')
    plt.grid(True)
    plt.show()