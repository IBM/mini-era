
This is the Keras code for CNN on MIO dataset.
The dataset is available on Rivendell.watson.ibm.com in /opt
0-background
1-bus
2-car
3-pedestrian
4-truck

Accuracy so far - 94%

python3 mio_dataset.py -- to parse dataset into .npy files
python3 mio_training.py -- for  training
python3 mio_inferece.py -- for inference

mio.py is the code called by keras_api.c. To run mio.py directly 
cd <mini_era_MAIN_folder>
python3 mio.py -t <object class 0 to 4>

