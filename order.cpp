#include"order.h"
#include"util.h"
#include"fileIO.h"
#include<stdlib.h>
#include<string.h>


Order::Order(Package *p, int i) {
	type = i;
	package = p;
	next = NULL;
	prior = NULL;
	wait_time = 0;
	depart_time = 0;
	current_weight = 0;
	prior_dis = 0.0;
	next_dis = 0.0;
	Dst = 10000000.0;
	cost = 100000000;
	visit = 0;
	if (i) {//�ö���Ϊ�յ�
		position = p->dest;
	}
	else {//�ö���Ϊ���
		position = p->origin;
	}
}

bool Order::check_order(char *id) {
	if (id[0] == 'F')
		return true;
	else
		return false;
}

void Order::dis_Dst_update(double prior_dis, double next_dis) {
	if (this->type) {
		package->current_dis += prior_dis + next_dis;
		package->Dst += this->Dst;
		//������ʷ���λ�þ���
		if (package->best_dis > package->current_dis)
			package->best_dis = package->current_dis;
	}
	else {
		package->current_dis = prior_dis + next_dis;
		package->Dst = this->Dst;
	}
}

int Order::dep_calc(PointOrder prior_order, int delta) {
	int new_dep = 0;
	double dis = 0.0;
	//���㶩����ǰ�̶����ľ���dis2
	if (prior_order == NULL) {
		if (check_order(package->id)) {
		//����Ϊ���̶���
			dis = 0;
		}
		else {
		//����Ϊoto����
			if (type) {
				dis = 0;
			}
			else {
				dis = Util::get_distance(position, FileIO::netpoint[1]);
				for (int i = 2; i < NET_NUM; i++) {
					double temp_dis = Util::get_distance(position, FileIO::netpoint[i]);
					if (dis > temp_dis)
						dis = temp_dis;
				}
			}
		}
		new_dep = Util::get_travel_time(dis);
	}
	else {
		dis = Util::get_distance(prior_order->position, position);
		//�����µ��뿪ʱ��
		new_dep = prior_order->depart_time + delta + Util::get_travel_time(dis);
	}
	if (type) {
		//��Ϊ�����յ�,����ϰ�������ʱ��
		new_dep += package->service_time();
	}
	else {
		//����뿪ʱ���Ƿ�����ʱ�䴰
		if (new_dep < package->start_time) {
			new_dep = package->start_time;
		}
	}
	return new_dep;
}

double Order::Dst_calc(double dis, int dep_time, double ds_factor, double dt_factor) {
	double Ds = (dis - MIN_DS) / (MAX_DS - MIN_DS), Dt = 0;
	double sav = 0.0, k1 = 1, k2 = 1.5, k3 = 2;
	if (check_order(this->package->id)) {
		sav = dep_time;
	}
	else {
		if (dep_time < this->package->start_time) {
			sav = k2*(dep_time - this->package->start_time) + k1*(this->package->end_time - this->package->start_time);
		}
		else if (dep_time > this->package->end_time) {
			sav = k3*(this->package->end_time - dep_time);
		}
		else {
			sav = k1*(this->package->end_time - dep_time);
		}
	}
	Dt = (k1 * 720 - sav - MIN_DT) / (MAX_DT - MIN_DT);
	return ds_factor * Ds + dt_factor * Dt;
}