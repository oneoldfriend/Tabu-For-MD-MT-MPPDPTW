#pragma once

class Position{
public:
	double lng, lat;
	Position();
};
class Package{
public:
	char id[6], depot_id[5];
	int cost, weight, start_time, end_time;
	double current_dis, best_dis, Dst;
	Position origin, dest;
	Package();
	//����ð����ķ���ʱ��
	int service_time();
};