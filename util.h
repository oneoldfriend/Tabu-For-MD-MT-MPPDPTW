#pragma once
#include"package.h"
#include"order.h"
#include<map>
#include<string>

using namespace std;
#define EARTH_RADIUS 6378137//����뾶
#define PI 3.14159265358979323846 //Բ����
#define speed 250 //���Ա�ٶ�

class Util {
public:
	//��idתΪint�����
	static int char_to_int(char *a);
	static int string_to_int(string a);
	//������ʱ��תΪ������8��Ĳ�
	static int char_to_time(char *a);
	//�����������
	static double get_distance(Position pos1, Position pos2);
	//�����ʻdis����ʱ��
	static int get_travel_time(double dis);
	static double rad(double d);
	static double relatedness_calc(pair<PointOrder, int> order1, pair<PointOrder, int> order2,
		double dis_factor, double time_factor, double weight_factor, double vehicle_factor, double type_factor);
};
