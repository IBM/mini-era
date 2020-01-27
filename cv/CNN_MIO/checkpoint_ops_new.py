# wrote this code based on these tutorials - 
# https://www.tensorflow.org/api_docs/python/tf/train/Saver
# https://www.tensorflow.org/programmers_guide/saved_model
# NC
## This file contains all check point related operations. 
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import sys
import csv
import numpy as np
import string
import os
import os.path
from tensorflow.python import pywrap_tensorflow
from tensorflow.python.platform import app
from tensorflow.python.platform import flags
#from tensorflow.python.summary import event_accumulator as ea
import pdb
import tensorflow as tf
from numpy import genfromtxt

FLAGS = None
DEBUG = 0
STR_REPLACE = 0	#Set to 1 if / does not work in variable name

# This function reads all tensors in the check point file, and returns the names and count of all tensors in the check point file
def return_tensor_names_in_checkpoint_file(file_name):
  try:
    reader = pywrap_tensorflow.NewCheckpointReader(file_name)  
  except:
    print("Check point file does not exist")
    sys.exit(-1)

  var_to_shape_map = reader.get_variable_to_shape_map()
  tensor_count = 0
  tensor_names=[]
  for tname in sorted(var_to_shape_map):
    tensor_names.append(tname)
    tensor_count=tensor_count+1

  print(tensor_count, tensor_names)
  return tensor_count, tensor_names  


def read_ckpt_into_dict(tcount,tnames,file_name):
  ckpt_dict={}
  var_names=[]
  #print(file_name, tcount)
  #if DEBUG:	  
  #  print(tnames)
  reader = pywrap_tensorflow.NewCheckpointReader(file_name)  
  for t in range(0,tcount):
    fname1=str(tnames[t])
    if STR_REPLACE: 
      fname=fname1.replace("/", "_") # removing / from file names. Some tensor names are stored with name scopes with /.
    else:
      fname = fname1 
    var_names.append(fname)
    tensor_array = reader.get_tensor(tnames[t])
    
    if tnames[t] == "global_step":	
      var=np.array(tensor_array, dtype=np.int)
      ckpt_dict[fname]=var
    else:
      var=np.array(tensor_array, dtype=np.float32)	
      ckpt_dict[fname]=var
  for key,val in ckpt_dict.items():
      print(key)
      print(val)
  return ckpt_dict, var_names		
	
def visualize_checkpoint(varname, var):
  tf.summary.histogram(varname, var)	

def gen_visualized_summary(sess,dfile):
  CWD=os.getcwd() 
  summary = tf.summary.merge_all()
  summary_writer = tf.summary.FileWriter(CWD, sess.graph)
  summary_str = sess.run(summary)
  summary_writer.add_summary(summary_str)
  #s=ea.EventAccumulator(dfile)
  summary_writer.flush()	

	
# This function doesn't work unfortunately because the check point file gets overwritten. 		
#def write_checkpoint(tcount,tnames,file_name):
#  tf.reset_default_graph()
#  init = tf.global_variables_initializer()
#
#  with tf.Session() as sess:
#
#    for t in range(1,tcount):
#      fname1=str(tnames[t])
#      fname=fname1.replace("/", "_") # removing / from file names. Some tensor names are stored with name scopes with /. 
#      fname="faulty_csv_files/"+fname+".csv"
#      np_var = genfromtxt(fname, delimiter=',')
#     # print(np_var)	  
#
#      with tf.variable_scope(tnames[t]):
#        v1 = tf.get_variable(tnames[t], initializer=np_var, dtype=tf.float64)
#        tf.add_to_collection("my_collection",v1)
#        saver = tf.train.Saver({tnames[t]:v1})
#        sess.run(init)
#        v1.initializer.run()
#        print("Variable value : %s" % v1.eval(session=sess))
#        saver.save(sess,file_name)   # this overwrites the check point file rather than append to it. :( 
#    #print(tf.get_collection("my_collection"))		
#


def read_checkpoint(ckpt_file):
  tcount, tnames = return_tensor_names_in_checkpoint_file(ckpt_file)
  #print(tcount, tnames)
  ckpt_dict, var_names = read_ckpt_into_dict(tcount,tnames,ckpt_file)
  #print(tcount, tnames, var_names, ckpt_dict)
  return tcount, tnames, var_names, ckpt_dict
  
  
def write_checkpoint(tcount,tnames,var_names,ckpt_dict,file_name):
  
  tf.reset_default_graph()
  init = tf.global_variables_initializer()
  print("Writing checkpoint: Tcount: " +str(tcount))
  print("Tnames: " +str(tnames[0]))
  with tf.Session() as sess:
    if (tcount == 5):
      v0 = tf.get_variable(tnames[0], initializer=ckpt_dict[var_names[0]])
      v1 = tf.get_variable(tnames[1], initializer=ckpt_dict[var_names[1]])
      v2 = tf.get_variable(tnames[2], initializer=ckpt_dict[var_names[2]])
      v3 = tf.get_variable(tnames[3], initializer=ckpt_dict[var_names[3]])
      v4 = tf.get_variable(tnames[4], initializer=ckpt_dict[var_names[4]])
    
      saver = tf.train.Saver({tnames[0]:v0, tnames[1]:v1, tnames[2]:v2, tnames[3]:v3, tnames[4]:v4})
      sess.run(init)
      v0.initializer.run()
      v1.initializer.run()
      v2.initializer.run()
      v3.initializer.run()
      v4.initializer.run()
      
      if DEBUG:	  
	      print("DEBUG : Variable value : %s" % v4.eval(session=sess))
      saver.save(sess,file_name)   
  
    elif (tcount == 232):  	#for resnets --- hardcoding for now
      print("Writing checkpoint for resnet 13")
      varname='v'
      saver_var=''
      for i in range(0,tcount):
        #tmp=tf.get_variable(tnames[0], initializer=ckpt_dict[var_names[0]]) 
        #exec(varname +str(i)+"="+str(tmp))
        #print(str(varname) +str(i)+"=tf.get_variable(tnames["+str(i)+"], initializer=ckpt_dict[var_names["+str(i)+"]])")
        exec(str(varname) +str(i)+"=tf.get_variable(tnames["+str(i)+"], initializer=ckpt_dict[var_names["+str(i)+"]])", globals(), locals())
        #saver_var=str(saver_var)+"tnames["+str(i)+":v"+str(i)+"],"
      #print(saver_var)
      #saver = tf.train.Saver({tnames[0]:v0, tnames[1]:v1, tnames[2]:v2, tnames[3]:v3, tnames[4]:v4, tnames[5]:v5, tnames[6]:v6, tnames[7]:v7, tnames[8]:v8})

      saver = tf.train.Saver({tnames[0]:v0,tnames[1]:v1,tnames[2]:v2,tnames[3]:v3,tnames[4]:v4,tnames[5]:v5,tnames[6]:v6,tnames[7]:v7,tnames[8]:v8,tnames[9]:v9,tnames[10]:v10,tnames[11]:v11,tnames[12]:v12,tnames[13]:v13,tnames[14]:v14,tnames[15]:v15,tnames[16]:v16,tnames[17]:v17,tnames[18]:v18,tnames[19]:v19,tnames[20]:v20,tnames[21]:v21,tnames[22]:v22,tnames[23]:v23,tnames[24]:v24,tnames[25]:v25,tnames[26]:v26,tnames[27]:v27,tnames[28]:v28,tnames[29]:v29,tnames[30]:v30,tnames[31]:v31,tnames[32]:v32,tnames[33]:v33,tnames[34]:v34,tnames[35]:v35,tnames[36]:v36,tnames[37]:v37,tnames[38]:v38,tnames[39]:v39,tnames[40]:v40,tnames[41]:v41,tnames[42]:v42,tnames[43]:v43,tnames[44]:v44,tnames[45]:v45,tnames[46]:v46,tnames[47]:v47,tnames[48]:v48,tnames[49]:v49,tnames[50]:v50,tnames[51]:v51,tnames[52]:v52,tnames[53]:v53,tnames[54]:v54,tnames[55]:v55,tnames[56]:v56,tnames[57]:v57,tnames[58]:v58,tnames[59]:v59,tnames[60]:v60,tnames[61]:v61,tnames[62]:v62,tnames[63]:v63,tnames[64]:v64,tnames[65]:v65,tnames[66]:v66,tnames[67]:v67,tnames[68]:v68,tnames[69]:v69,tnames[70]:v70,tnames[71]:v71,tnames[72]:v72,tnames[73]:v73,tnames[74]:v74,tnames[75]:v75,tnames[76]:v76,tnames[77]:v77,tnames[78]:v78,tnames[79]:v79,tnames[80]:v80,tnames[81]:v81,tnames[82]:v82,tnames[83]:v83,tnames[84]:v84,tnames[85]:v85,tnames[86]:v86,tnames[87]:v87,tnames[88]:v88,tnames[89]:v89,tnames[90]:v90,tnames[91]:v91,tnames[92]:v92,tnames[93]:v93,tnames[94]:v94,tnames[95]:v95,tnames[96]:v96,tnames[97]:v97,tnames[98]:v98,tnames[99]:v99,tnames[100]:v100,tnames[101]:v101,tnames[102]:v102,tnames[103]:v103,tnames[104]:v104,tnames[105]:v105,tnames[106]:v106,tnames[107]:v107,tnames[108]:v108,tnames[109]:v109,tnames[110]:v110,tnames[111]:v111,tnames[112]:v112,tnames[113]:v113,tnames[114]:v114,tnames[115]:v115,tnames[116]:v116,tnames[117]:v117,tnames[118]:v118,tnames[119]:v119,tnames[120]:v120,tnames[121]:v121,tnames[122]:v122,tnames[123]:v123,tnames[124]:v124,tnames[125]:v125,tnames[126]:v126,tnames[127]:v127,tnames[128]:v128,tnames[129]:v129,tnames[130]:v130,tnames[131]:v131,tnames[132]:v132,tnames[133]:v133,tnames[134]:v134,tnames[135]:v135,tnames[136]:v136,tnames[137]:v137,tnames[138]:v138,tnames[139]:v139,tnames[140]:v140,tnames[141]:v141,tnames[142]:v142,tnames[143]:v143,tnames[144]:v144,tnames[145]:v145,tnames[146]:v146,tnames[147]:v147,tnames[148]:v148,tnames[149]:v149,tnames[150]:v150,tnames[151]:v151,tnames[152]:v152,tnames[153]:v153,tnames[154]:v154,tnames[155]:v155,tnames[156]:v156,tnames[157]:v157,tnames[158]:v158,tnames[159]:v159,tnames[160]:v160,tnames[161]:v161,tnames[162]:v162,tnames[163]:v163,tnames[164]:v164,tnames[165]:v165,tnames[166]:v166,tnames[167]:v167,tnames[168]:v168,tnames[169]:v169,tnames[170]:v170,tnames[171]:v171,tnames[172]:v172,tnames[173]:v173,tnames[174]:v174,tnames[175]:v175,tnames[176]:v176,tnames[177]:v177,tnames[178]:v178,tnames[179]:v179,tnames[180]:v180,tnames[181]:v181,tnames[182]:v182,tnames[183]:v183,tnames[184]:v184,tnames[185]:v185,tnames[186]:v186,tnames[187]:v187,tnames[188]:v188,tnames[189]:v189,tnames[190]:v190,tnames[191]:v191,tnames[192]:v192,tnames[193]:v193,tnames[194]:v194,tnames[195]:v195,tnames[196]:v196,tnames[197]:v197,tnames[198]:v198,tnames[199]:v199,tnames[200]:v200,tnames[201]:v201,tnames[202]:v202,tnames[203]:v203,tnames[204]:v204,tnames[205]:v205,tnames[206]:v206,tnames[207]:v207,tnames[208]:v208,tnames[209]:v209,tnames[210]:v210,tnames[211]:v211,tnames[212]:v212,tnames[213]:v213,tnames[214]:v214,tnames[215]:v215,tnames[216]:v216,tnames[217]:v217,tnames[218]:v218,tnames[219]:v219,tnames[220]:v220,tnames[221]:v221,tnames[222]:v222,tnames[223]:v223,tnames[224]:v224,tnames[225]:v225,tnames[226]:v226,tnames[227]:v227,tnames[228]:v228,tnames[229]:v229,tnames[230]:v230,tnames[231]:v231})

      sess.run(init)

      for i in range(0,tcount):
        exec(str(varname) +str(i)+".initializer.run()")

      if DEBUG:	  
	      print("DEBUG : Variable value : %s" % v231.eval(session=sess))

      saver.save(sess,file_name)   

    else:
      print("Add more tensor count configurations,cannot process anything other than 5 7 or 9 arrays in the checkpoint")




	
def main(_):
   global FLAGS
   if (FLAGS.parse_checkpoint):
     tcount, tnames = return_tensor_names_in_checkpoint_file(FLAGS.read_ckpt_file_name)
#     read_ckpt_into_csvfiles(tcount,tnames,FLAGS.read_ckpt_file_name)
     read_ckpt_into_dict(tcount,tnames,FLAGS.read_ckpt_file_name)
   if (FLAGS.create_checkpoint):
     write_checkpoint(tcount,tnames,FLAGS.write_ckpt_file_name)
        

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
 
  parser.add_argument(
      '--read_ckpt_file_name',
      type=str,
      default='./mnist/checkpoint/model.ckpt-49999',
      help='Checkpoint filename that needs to be read'
    )
  parser.add_argument(
      '--parse_checkpoint',
      default=True,
      help='If true, parses a check point file',
      action='store_true'
  )

  parser.add_argument(
      '--create_checkpoint',
      default=False,
      help='If true, creates a check point file from np arrays in multiple csv files',
      action='store_true'
  )
  parser.add_argument(
      '--write_ckpt_file_name',
      type=str,
      default='./mnist/checkpoint/test.ckpt',
      help='Checkpoint filename to be saved'
    )
  parser.add_argument(
      '--tensor_dir',
      type=str,
      default='./tensor_csv_files',
      help='Directory where tensors need to be read from check point and written to in txt files'
    )
  parser.add_argument(
      '--faulty_tensor_dir',
      type=str,
      default='./faulty_tensor_csv_files',
      help='Directory where faulty tensors txt files exist'
    )
  
  FLAGS, unparsed = parser.parse_known_args()
  tf.app.run(main=main, argv=[sys.argv[0]] + unparsed)	
