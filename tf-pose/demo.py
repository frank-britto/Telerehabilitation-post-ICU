# Importar librerías a utilizar
import sys 
import logging # Detectar errores (recomendado en https://realpython.com/python-logging/)
import numpy as np
import matplotlib.pyplot as plt
import cv2
import time

# Funciones propias de tf-open-estimation
from tf_pose import common
from tf_pose.estimator import TfPoseEstimator
from tf_pose.networks import get_graph_path, model_wh

# Inicialización de variables especiales
""" De acuerdo a https://github.com/ildoonet/tf-pose-estimation, el modelo con menor
latencia es mobilenet_thin"""
model = 'mobilenet_thin'
resize = '432x368' # pre procesamiento de imágenes (escalamiento)
resize_out_ratio = 4.0 # mapas de calor
show_process = False
tensorrt = False
fps_time = 0
w,h = model_wh(resize)

# Instance de tf-pose
if w > 0 and h > 0:
    e = TfPoseEstimator(get_graph_path(model), target_size = (w,h), trt_bool = False)
else:
    e = TfPoseEstimator(get_graph_path(model), target_size=(432,368), trt_bool = False)

# Funciones auxiliares
def get_human_pose(ruta_imagen, BG = True):

    """ Creación del esqueleto de colores"""
    
    image = common.read_imgfile(ruta_imagen, None, None) # Mejor que cv2.imread()

    if image is None:
        logger.error('No se encontró la imagen en la ruta = %s' % image)
        sys.exit(-1)

    humans = e.inference(image, resize_to_default=(w>0 and h>0), upsample_size=4.0)

    if BG == False:
        image = np.zeros(image,shape)

    image = TfPoseEstimator.draw_humans(image, humans, imgcopy = False)
    return image,humans

def joint_keypoints(image, hum, human = 1, color = "white", BG = True):

    """ Recuperar keypoints de las diferentes articulaciones"""

    if human == 0:
        human = 1
    num_hum = len(hum)

    print(hum)
    
    keypoints = str(str(str(hum[human-1]).split('BodyPart:')[1:]).split('-')).split(' score=')
    #print('Raw data \n')
    #print(keypoints) #-> raw data
    
    keypoints_list = []
    for i in range(len(keypoints)-1):
        pts = keypoints[i][-11:-1]
        pts = tuple(map(float,pts.split(',')))
        keypoints_list.append(pts)
    #print('\nPreprocesamiento\n')
    #print(keypoints_list) #-> ordenar raw data

    joints = np.array(keypoints_list)
    joints = joints*(image.shape[1], image.shape[0])
    joints = joints.astype(int)
    #print('\nCoordenadas en pixeles')
    #print(joints) #-> coordenadas en pixeles

    
    plt.figure(figsize=(10,10))
    plt.axis([0, image.shape[1], 0, image.shape[0]])  
    plt.scatter(*zip(*joints), s=200, color=color, alpha=0.6)

    if BG:
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    plt.imshow(image)
    ax=plt.gca() 
    ax.set_ylim(ax.get_ylim()[::-1]) 
    ax.xaxis.tick_top()
    plt.title('Keypoints detectados')
    plt.grid()

    for i, txt in enumerate(joints):
        ax.annotate(i, (joints[i][0]-5, joints[i][1]+5))

    return keypoints_list
        

