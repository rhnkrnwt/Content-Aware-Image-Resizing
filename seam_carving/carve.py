import numpy as np
import cv2
import sys
import imutils

def resize_image(input_image_path, output_height, output_width):
    print 'Resizing...'
    #read image and convert to float 64 for manipulation
    input_image = cv2.imread(input_image_path).astype(np.uint8)
    #get height and width to compare to new size
    old_height, old_width = input_image.shape[: 2]
    #get total change in rows and columns so we know how to resize
    delta_col, delta_row = output_width - old_width, output_height - old_height
    if np.absolute(delta_row) > 0:
        #rotate image i necesssary
        input_image = rotate_image(input_image, 90)
        delta_col = delta_row
    #for use later
    cv2.imwrite('seam_overlay.png', input_image)
    #create a copy of original to manipulate for our output image
    output_image = np.copy(input_image)
    #get energy map of image
    energy_map = generate_energy_map(output_image)
    #apply color map to create transport
    color_img = cv2.applyColorMap(energy_map, cv2.COLORMAP_JET)
    #save color map image
    cv2.imwrite('energy_color.png', color_img)
    #optional, print energy map
    cv2.imwrite('energy_map.png', energy_map)
    output_image_name = ''
    #if necessary to vertical cut, do so
    if delta_col > 0:
        output_image_name = 'expanded_' + str(output_width) + '_final'
        #our intial image that we will now expand
        expanded_image = np.copy(input_image)
        total_seams = []
        for dummy in range(delta_col):
            #traverse through rows to find minimum values
            seam = find_seam(output_image, energy_map)     
            #write seam to overlay image
            write_seam(seam, 'seam_overlay.png')
            #add seam to our seams array
            total_seams.append(seam)
            #delete the seam to find the next one minimum
            output_image = delete_seam(output_image, seam)
            #calculate new energy map of image
            energy_map = generate_energy_map(output_image)
        num_seams = len(total_seams)
        #overwire our expanded image
        expanded_image = np.copy(input_image)
        for dummy in range(num_seams):
            seam = total_seams.pop(0)
            expanded_image = insert_seam(seam, expanded_image)
            total_seams = update_seams(total_seams, seam)
        #revert if we rotated
        if np.absolute(delta_row) > 0:
            expanded_image= rotate_image(expanded_image, -90)
        save_final_img(expanded_image, output_image_name) 
    else:
        output_image_name = 'reduced_' + str(output_width) + '_final'
        #case for needing to remove columns (make image narrower)
        delta_col = np.absolute(delta_col)
        #iterate through and remove seams of low energy pixels
        for dummy in range(delta_col):
            #traverse through rows to find minimum values
            seam = find_seam(output_image, energy_map)
            #write seam to overaly image
            write_seam(seam, 'seam_overlay.png')
            #delete the seam to find the next one
            output_image = delete_seam(output_image, seam)
            energy_map = generate_energy_map(output_image)
        #revert if we rotated
        if np.absolute(delta_row) > 0:
            output_image = rotate_image(output_image, 270)
        save_final_img(output_image, output_image_name)
    return output_image

def rotate_image(image, angle):
	rotated = imutils.rotate_bound(image, angle)
	return rotated

def find_seam(img, energy_map):
    #get the size of our current image
    rows, cols = img.shape[:2]
    #create a seam that has the height of the image but is empty
    seam = np.zeros(img.shape[0]) 
    distance = np.zeros(img.shape[:2]) + sys.maxint
    distance[0,:] = np.zeros(img.shape[1])
    edge = np.zeros(img.shape[:2])
    for row in range(rows-1):
        for col in range(cols):
            #apply the energy formulat here
            #M(i, j) = e(i, j) + min(M(i-1, j-1), M(i-1, j), M(i-1, j + 1))
            #not first row
            if col != 0:
                #caclue the column behind
                first = distance[row+1, col-1]
                prev = distance[row, col] + energy_map[row+1, col-1]
                #set to minimum of the two
                distance[row + 1, col - 1] = min(first, prev)
                #we are on the edge
                if first > prev:
                    edge[row+1, col-1] = 1
            #calcualte the existing column 
            second = distance[row+1, col]
            current = distance[row, col] + energy_map[row+1, col]
            #set to minimum of the two
            distance[row + 1, col] = min(second, current)
            if second > current:
                #not on the edge
                edge[row+1, col] = 0
            #not next to edge
            if col != cols-1:
                third = distance[row+1, col+1]
                next = distance[row, col] + energy_map[row+1, col+1]
                distance[row+1, col+1]  = min(third, next)
                if third > next:
                    edge[row+1, col+1] = -1

    # Retrace the min-path and update the 'seam' vector
    seam[rows-1] = np.argmin(distance[rows-1, :])
    for i in (x for x in reversed(range(rows)) if x > 0):
        seam[i-1] = seam[i] + edge[i, int(seam[i])]
    #we found our optimal seam
    return seam

def insert_seam(seam, img):
    rows, cols = img.shape[:2]
    #zeros
    zeros = np.zeros((rows,1,3), dtype=np.uint8)
    #updated image with additional pixels
    img_extended = np.hstack((img, zeros))
    for row in range(rows):
        #find where seam should be sinerted
        for col in range(cols, int(seam[row]), -1):
            img_extended[row, col] = img[row, col-1]
        #insert the average of its two neighbors
        for i in range(3):
            v1 = img_extended[row, int(seam[row])-1, i]
            v2 = img_extended[row, int(seam[row])+1, i]
            #calculate the average and set equal to the values
            img_extended[row, int(seam[row]), i] = (int(v1)+int(v2))/2
    return img_extended

def delete_seam(image, seam):
    rows, cols = image.shape[:2]
    #remove seam from image and adjust all other cols
    for row in range(rows):
        #remove seam from imae and move over all other cols
        for col in range(int(seam[row]), cols-1):
            image[row, col] = image[row, col+1]
    small_image = image[:, 0:cols-1]
    return small_image

def save_final_img(img, name):
    cv2.imwrite(name + '_output.png', img.astype(np.uint8))

def update_seams(remaining, current):
    seams = []
    #so we know what seams have been used and move over the other ones
    for seam in remaining:
        #move over seam by 2 pixels 
        seam[np.where(seam >= current)] += 2
        #return updated seams list wit updated location
        seams.append(seam)
    return seams

#show the line of the seam discovered on the picture
def write_seam(seam, img_path):
    #get existing image that has previous written seams
    img = cv2.imread(img_path)
    seam_img = np.copy(img) 
    x_coords, y_coords = np.transpose([(i,int(j)) for i,j in enumerate(seam)])
    #make the seam red
    seam_img[x_coords, y_coords] = (0,0,255)
    #save to image so we can see seams
    cv2.imwrite(img_path, seam_img)
    return seam_img

def generate_energy_map(img):
    # optinoally apply blur
    img = cv2.GaussianBlur(img,(3,3),0).astype(np.int8)
    #turn in into gray to generate x and y gradients
    gray = cv2.cvtColor(img.astype(np.uint8), cv2.COLOR_BGR2GRAY)
    #gradient x
    sobel_x = cv2.Sobel(gray,cv2.CV_64F,1,0,ksize=3)
    #gradient 7
    sobel_y = cv2.Sobel(gray,cv2.CV_64F,0,1,ksize=3)
    #abs value of x
    abs_sobel_x = cv2.convertScaleAbs(sobel_x)
    #abs value of y
    abs_sobel_y = cv2.convertScaleAbs(sobel_y)
    #merged energy map
    energy_map = cv2.addWeighted(abs_sobel_x, 0.5, abs_sobel_y, 0.5, 0)
    return energy_map

#main function
if __name__ == '__main__':
    image_input = raw_input("Name of original image: ")
    path = image_input
    img = cv2.imread(path)
    print "Current dimensions (HxW): " + str(img.shape[:2])    
    output_height = raw_input("Resize height to: ")
    output_width = raw_input("Resize width to: ")
    #resize image
    resize_image(path, int(output_height), int(output_width))
    print('Done - output displayed in output folder.')
