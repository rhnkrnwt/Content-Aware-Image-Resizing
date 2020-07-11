from numpy import *
from scipy import misc
from scipy.stats import norm
import matplotlib.pyplot as plt
import math
from sklearn.feature_extraction import image
import time
import pylab as plt1
from test import *
from patchmatch_code import *
import cv2

w_patch=3
m_patch = 2*w_patch+1
knn=1

def offset_dict(offset, im, im_t):
    offset.astype(int)
    [M, N] = im.shape
    [M_t, N_t] = im_t.shape
    n = M*N
    n_t = M_t*N_t
    offsets = {}
    for i in range(0,n):
        idx = i + offset[i]
        idx = max(0, idx)
        idx = min(idx, n_t-1)
        j = int(idx)
        if j in offsets:
            offsets[j].append(i)
        else:
            offsets[j] = []
            offsets[j].append(i)
    return offsets


def color(idx, im, im_t, offsets, offsets_t):
    [M, N] = im.shape
    [M_t, N_t] = im_t.shape
    n = M*N
    n_t = M_t*N_t
    [i, j] = idx1d_to_idx2d(idx, M_t, N_t)
    co1=0.0
    sum1=0.0
    for r in range( max(0, i-w_patch), min(i+w_patch+1, M_t) ):
        for c in range( max(0, j-w_patch), min(j+w_patch+1, N_t) ):
            co1+=1
            idx1 = idx2d_to_idx1d(r, c, M_t, N_t) #pacth idx1 containing idx
            idx2 = idx1 + offsets_t[idx1] #patch in source which is nn of idx1 patch
            idx2 = max(0, idx2)
            idx2 = min(idx2, n-1)
            [i2, j2] = idx1d_to_idx2d(idx2, M, N)
            [i3, j3] = [i2+(r-i), j2+(c-j)]
            i3 = max(0, i3)
            i3 = min(i3, M-1)
            j3 = max(0, j3)
            j3 = min(j3, N-1)
            sum1+= im[i3, j3]
    sum1/= n_t
    co2=0.0
    sum2=0.0

    for r in range( max(0, i-w_patch), min(i+w_patch+1, M_t) ):
        for c in range( max(0, j-w_patch), min(j+w_patch+1, N_t) ):
            idx1 = idx2d_to_idx1d(r, c, M_t, N_t) #pacth idx1 containing idx
            if idx1 in offsets:
                for t in range(0, len(offsets[idx1]) ):
                    co2+=1
                    idx2 = offsets[int(idx1)][t] #patch in source which is nn of idx1 patch
                    idx2 = max(0, idx2)
                    idx2 = min(idx2, n-1)
                    [i2, j2] = idx1d_to_idx2d(idx2, M, N)
                    [i3, j3] = [i2+(r-i), j2+(c-j)]
                    i3 = max(0, i3)
                    i3 = min(i3, M-1)
                    j3 = max(0, j3)
                    j3 = min(j3, N-1)
                    sum2+= im[i3, j3]
    sum2/= n
    return (sum1+sum2)/(co1/n_t + co2/n)

def updation(im, im_t, offsets, offsets_t):
    [M, N] = im.shape
    [M_t, N_t] = im_t.shape
    n = M*N
    n_t = M_t * N_t
    for i in range(0,n_t):
        [i0, j0] = idx1d_to_idx2d(i, M_t, N_t)
        im_t[i0,j0] = color(i, im, im_t, offsets, offsets_t)
    return im_t

def iterations(im):
        c=0
        im_t = im
    #while im_t.shape[0]/float(im.shape[0]) >= 0.75 and im_t.shape[1]/float(im.shape[1]) >= 0.75:
        c+=1
        print(str(c)+"-iteration started")
        t0 = time.time()
        im_t = misc.imresize(im_t, 0.95)
        im_t = 255.*im_t/im_t.max()
        offset = patch_match(im_t, im, m_patch, knn)
        offsets_t = patch_match(im, im_t, m_patch, knn)
        offsets = offset_dict(offset, im, im_t)
        im_t = updation(im, im_t, offsets, offsets_t)
        for iter in range(9):
            print(iter)
            offset = patch_match(im_t, im, m_patch, knn)
            offsets_t = patch_match(im, im_t, m_patch, knn)
            offsets = offset_dict(offset, im, im_t)
            im_t = updation(im, im_t, offsets, offsets_t)
            dpi = 80.0
            plt.figure(figsize=(im_t.shape[0]/dpi, im_t.shape[1]/dpi), dpi=dpi)
            imgplot=plt.imshow(im_t, cmap='gray', vmin = 0, vmax = 255)
            #plt.show()
            cv2.imshow('image',im_t)
            cv2.imwrite('figure'+str(c)+str(iter)+'.png',im_t)
            cv2.destroyAllWindows()
        t1 = time.time()
        print(str(c)+"-iteration took " + str(t1-t0) + " s. \n")
        dpi = 80.0
        plt.figure(figsize=(im_t.shape[0]/dpi, im_t.shape[1]/dpi), dpi=dpi)
        imgplot=plt.imshow(im_t, cmap='gray', vmin = 0, vmax = 255)
        #plt.show()
        cv2.imshow('image',im_t)
        cv2.imwrite('figure'+str(c)+'.png',im_t)
        cv2.destroyAllWindows()
        #plt.imsave(im_t, cmap='gray', vmin = 0, vmax = 255)


if __name__ == "__main__":

    t0 = time.time()
    im_name = 'img2_1.png'
    img = cv2.imread('img2_1.png',0)
    im = misc.imread(im_name, 'L')
    im = 255.*im/im.max()
    dpi = 80.0
    #imgplot = plt.imshow(im)
    #plt.show()
    c=0
    plt.figure(figsize=(im.shape[0]/dpi, im.shape[1]/dpi), dpi=dpi)
    imgplot=plt.imshow(im, cmap='gray', vmin = 0, vmax = 255)
    cv2.imshow('image',im)
    cv2.imwrite('figure'+str(c)+'.png',im)
    cv2.destroyAllWindows()
    #misc.imsave("out.png", im,cmap='gray', vmin = 0, vmax = 255)
    #plt.save(im, cmap='gray', vmin = 0, vmax = 255)
    iterations(im)
    t1 = time.time()
    print("Total Execution took " + str(t1-t0) + " s. \n")
    plt.show()
