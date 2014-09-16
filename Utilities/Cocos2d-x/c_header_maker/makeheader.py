#!/usr/bin/python
# -*- coding: utf-8 -*-
import os 
import sys 
import glob
import os.path
#XML reader
from xml.dom import minidom

header_add_path = 'data\\'

def makeheader():
	print u"start . make header file"
	
	param = sys.argv

	path = os.getcwd()
	os.chdir(param[1])
	list = glob.glob('*.c')
	f = open('ssdatas.cpp' ,'w' )
	f2 = open('ssdatas.h' ,'w' )
	
	f.writelines( '#ifndef __TESTCASE__\n' )
	f.writelines( '#define __TESTCASE__\n' )
	f.writelines('\n')
	f.writelines( '#include "SSPlayerData.h"\n' )
	f.writelines('\n')
	f.writelines('\n')
	
	f2.writelines('#include "SSPlayerData.h"\n')
	
	
	for file in list:
		str = '#include "' + header_add_path + file + '"' + "\n"
		f.writelines(str)
		name , ext = os.path.splitext( file )
		str = "extern const char* " + name + "_images[];\n"
		str = str + "extern SSData " + name + "_partsData;\n\n"
		f2.writelines(str)

	f.writelines('\n')
	f.writelines('\n')
	f2.writelines('\n')
	f2.writelines('\n')
	
	str = 'struct SsDataTable{\n'
	str+= '\tconst char* animename;\n'
	str+= '\tint width;\n'
	str+= '\tint height;\n'
	str+= '\tconst char** images;\n'
	str+= '\tSSData* ssdata;\n'
	str+= '};\n'
	str+= '\n';
	str+= '\n';
	
	f.writelines( str )
	f2.writelines( str )
	
	str = 'SsDataTable ssdatatable[]={\n'

	for file in list:
		name , ext = os.path.splitext( file )
		#ssaxのキャンバスサイズを取得する
		print name +".ssax"
		xdoc = minidom.parse( name +".ssax" )
		width = xdoc.getElementsByTagName("CanvasWidth").item(0).childNodes[0].data
		height = xdoc.getElementsByTagName("CanvasHeight").item(0).childNodes[0].data
		#情報を書き込む
		str+= '\t{ ' + '"' + name + '"\t,' + width +',' + height + ',\t' + name + '_images ,\t &' + name + '_partsData } ,\n'
	
	str+= '\t{ 0 , 0 , 0 , 0 , 0 },\n'
	str+= '};\n'
	str+= '\n';
	f.writelines( str )
	f2.writelines( 'extern SsDataTable ssdatatable[];' );
	
	f.writelines( '#endif\n' )
	f.close()
	os.chdir(path)
	
	print u"end . make header file"
	
if __name__ == "__main__":
	makeheader()

