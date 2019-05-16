#include"alns.h"
#include"fileIO.h"
#include<iostream>
#include<string.h>
#include<stdio.h>
#include"util.h"
#include<utility>
#include<string>
#include<cstdlib>
#include<cmath>
#include<time.h>
#include<algorithm>
#include<map>
using namespace std;


Solution Alns::initial;
Solution Alns::best;
Solution Alns::news;
OperatorInfo Alns::del_operator[DEL_NUM] = { OperatorInfo() };
OperatorInfo Alns::ins_operator[INS_NUM] = { OperatorInfo() };
OperatorInfo Alns::tabu_operator[TABU_OPRT] = { OperatorInfo() };
DeleteList Alns::delete_list[LIST_SIZE] = { DeleteList() };
map<string, int> Alns::two_opt_tabu;
map<string, int> Alns::swap_tabu;
map<string, int> Alns::reinsert_tabu;
map<string, int> Alns::global_tabu;
double Alns::tabu_size_rate;
double Alns::list_size_rate;
int Alns::oto_num;
int Alns::eco_num;
map<string, double> Alns::depot_lb;
map<string, double> Alns::alns_touched_order;
map<string, double> Alns::tabu_touched_order;


DeleteList::DeleteList() {
	id[0] = '\0';
}
OperatorInfo::OperatorInfo(){
	times = 1;
	score = 1;
	prob = 0;
}

void Alns::alns(int input_eco_num, int input_oto_num, int input_run_num) {
	eco_num = input_eco_num;
	oto_num = input_oto_num;
	int data_size = eco_num + oto_num;
	char soluion[] = { data_size / 1000 + 48,data_size % 1000 / 100 + 48,data_size % 100 / 10 + 48,data_size % 10 + 48,'-',input_run_num + 48,'-', '\0' };
	string output_solution = "full_solution.csv";
	string output_data = "tabu-forbid-both.csv";
	string data_path = "data/";
	FileIO::netpoint_input(data_path);
	FileIO::delivery_input(data_path);
	FileIO::shop_input(data_path);
	FileIO::eorder_input(data_path);
	FileIO::otoorder_input(data_path);

	output_solution = data_path + output_solution;
	output_data = data_path + output_data;
	/*initial.get_initial();
	FILE *res = fopen("ecom_initial_no_vehicle_lmt", "w");
	FileIO::solution_output(initial, res);
	fclose(res);*/
	FILE *input = fopen("full_initial.csv", "r");
	FileIO::solution_input(initial, input, data_size);
	fclose(input);

	get_depot_lb();
	int best_cost = 0, initial_cost = 0, seg_count = 0, new_cost = 0, wait_time = 0, penalty = 0;
	initial.package_info_calc_update();
	for (int i = 0; i < COURIOR_NUM; i++) {
		initial_cost += initial.courior[i].cost;
		wait_time += initial.courior[i].wait_time;
		penalty += initial.courior[i].penalty;
	}
	initial.calcAttributes();
	solution_copy(&best, &initial);
	best_cost = initial_cost;
	new_cost = initial_cost;
	printf("initial cost:%d\nbest cost:%d\nwait time:%d\npenalty:%d\n", initial_cost, best_cost, wait_time, penalty);

	tabu_size_rate = 0.2;

	int iter_before_tabu = 0;
	for (int iteration = 0; iteration < 1; iteration++){
		FILE *data = fopen(output_data.c_str(), "w");
		fprintf(data, "%0.3lf,%d,%d,%d,%d,%d,%d\n", 0.00, best_cost, new_cost, wait_time, penalty,
				best.vehicleNumber, best.idleTime);
		int origin_cost = 0;
		vector<PointCourior> fresh_routes;
		for (int i = 0; i < initial.vehicleNumber * tabu_size_rate; i++){
			origin_cost += initial.courior[i].get_cost();
			fresh_routes.push_back(&initial.courior[i]);
		}
		Alns::tabu(fresh_routes, data, &best_cost);
		fclose(data);
	}
}

void Alns::removal_oprt_prob_reward()
{
	int num = 0, scale = (Alns::oto_num + Alns::eco_num)*list_size_rate;
	double total_prob = 0;
	while (num < DEL_NUM) {
		switch (num) {
		case 0: {
			news.random_removal();
			break;
		}
		case 1: {
			news.worst_cost_removal();
			break;
		}
		case 2: {
			news.depot_related_removal();
			break;
		}
		case 3: {
			news.wait_time_removal();
			break;
		}
		case 4: {
			news.spatio_removal();
			break;
		}
		case 5: {
			news.temporal_removal();
			break;
		}
		case 6: {
			news.spatiotemporal_removal();
			break;
		}
		case 7: {
			news.shaw_removal();
			break;
		}
		case 8: {
			news.dis_related_removal();
			break;
		}
		case 9: {
			news.time_related_removal();
			break;
		}
		default:
			break;
		}
		int touched_count = 0;
		for (int i = 0; i < scale; i++) {
			if (Alns::tabu_touched_order[Alns::delete_list[i].id] > 0)
				touched_count++;
			Alns::delete_list[i].id[0] = '\0';
		}
		Alns::del_operator[num].prob += double(touched_count) / scale;
		total_prob += Alns::del_operator[num].prob;
		num++;
	}
	if (total_prob == 0)
		return;
	for (int i = 0; i < DEL_NUM; i++) {
		Alns::del_operator[i].prob = Alns::del_operator[i].prob / total_prob;
	}
}

void Alns::tabu(vector<PointCourior> routes, FILE *fp, int *best_cost)
{
	int initial_cost = 0, new_cost = 0, penalty = 0, wait_time = 0, unimprovement = 0, iter_count = 0;
	double running_time = 0;
	while (iter_count < 1000) {
		double duration;
		clock_t start, end;
		start = clock();
		iter_count++;
		for (map<string, int>::iterator iter = Alns::two_opt_tabu.begin(); iter != Alns::two_opt_tabu.end(); ++iter) {
			if (iter->second > 0)
				iter->second--;
		}
		for (map<string, int>::iterator iter = Alns::reinsert_tabu.begin(); iter != Alns::reinsert_tabu.end(); ++iter) {
			if (iter->second > 0)
				iter->second--;
		}
		for (map<string, int>::iterator iter = Alns::swap_tabu.begin(); iter != Alns::swap_tabu.end(); ++iter) {
			if (iter->second > 0)
				iter->second--;
		}
		int oprt_num = roulette_wheel(3);
		switch (oprt_num) {
		case 0: {
			printf("using swap operator\n");
			Solution::swap(routes);
			Alns::tabu_operator[0].times++;
			break;
		}
		case 1: {
			printf("using reinsertion operator\n");
			Solution::reinsertion(routes);
			Alns::tabu_operator[1].times++;
			break;
		}
		case 2: {
			printf("using two-opt operator\n");
			Solution::two_opt(routes);
			Alns::tabu_operator[2].times++;
			break;
		}
		}
		new_cost = 0;
		penalty = 0;
		wait_time = 0;
		for (int i = 0; i < COURIOR_NUM; i++) {
			new_cost += initial.courior[i].cost;
			penalty += initial.courior[i].penalty;
			wait_time += initial.courior[i].wait_time;
		}
		initial.calcAttributes();
		if (new_cost < *best_cost) {
			*best_cost = new_cost;
			solution_copy(&best, &initial);
			unimprovement = 0;
			tabu_operator[oprt_num].score += sigma1;
		}
		printf("prev cost: %d\nnew cost: %d\nbest cost: %d\n", initial_cost, new_cost, *best_cost);
		if (new_cost >= initial_cost)
			tabu_operator[oprt_num].score += -sigma1;
		else
			tabu_operator[oprt_num].score += sigma2;
		tabu_oprt_update();
		initial_cost = new_cost;
		end = clock();
		duration = (double)(end - start) / CLOCKS_PER_SEC / 60;
		running_time += duration;
		printf("operator last:%0.1fmin\n\n", duration);
		fprintf(fp, "%0.3lf,%d,%d,%d,%d,%d,%d\n", running_time, *best_cost, new_cost, wait_time, penalty,
				best.vehicleNumber, best.idleTime);
	}
	printf("running time:%0.2lf\n", running_time * 60.0);
}

double Alns::get_list_size_rate(double new_rate)
{
	if (new_rate > MOVE_SIZE_MAX)
		return MOVE_SIZE_MAX;
	else if (new_rate < MOVE_SIZE_MIN)
		return MOVE_SIZE_MIN;
	else
		return new_rate;
}

void Alns::tabu_selection(vector<PointCourior> *tabu_route)
{
	list<pair<int, double> > TabuOrdersInRoute;
	for (int i = 0; i < COURIOR_NUM; i++) {
		double count_tabuOrder = 0, count_pathOrder = 0;
		PointOrder p = initial.courior[i].path->head;
		if (p != NULL) {
			while (p != NULL) {
				count_pathOrder++;
				if (Alns::global_tabu[p->package->id] > 0)
					count_tabuOrder++;
				p = p->next;
			}
			TabuOrdersInRoute.push_back(make_pair(i, count_tabuOrder / count_pathOrder));
		}
	}
	TabuOrdersInRoute.sort(route_sort_ascend);
	int oprt_route = round(TabuOrdersInRoute.size()*Alns::tabu_size_rate);
	int oprt_order = round((Alns::oto_num + Alns::eco_num)*Alns::tabu_size_rate);
	for (int i = 0, order_count = 0; i < oprt_route || order_count< oprt_order; i++) {
		tabu_route->push_back(&initial.courior[TabuOrdersInRoute.front().first]);
		PointOrder p = initial.courior[TabuOrdersInRoute.front().first].path->head;
		while (p != NULL) {
			if (p->type)
				order_count++;
			p = p->next;
		}
		TabuOrdersInRoute.pop_front();
	}
}

void Alns::cost_selection(vector<PointCourior> *tabu_route)
{
	list<pair<int, double> > route_cost_list;
	for (int i = 0; i < COURIOR_NUM; i++) {
		PointOrder p = initial.courior[i].path->head;
		if (p != NULL) {
			route_cost_list.push_back(make_pair(i, initial.courior[i].cost));
		}
	}
	route_cost_list.sort(route_sort_descend);
	int oprt_route = round(route_cost_list.size()*Alns::tabu_size_rate);
	int oprt_order = round((Alns::oto_num + Alns::eco_num)*Alns::tabu_size_rate);
	for (int i = 0, order_count = 0; i < oprt_route || order_count < oprt_order; i++) {
		tabu_route->push_back(&initial.courior[route_cost_list.front().first]);
		PointOrder p = initial.courior[route_cost_list.front().first].path->head;
		while (p != NULL) {
			if (p->type)
				order_count++;
			p = p->next;
		}
		route_cost_list.pop_front();
	}
}

void Alns::distance_selection(vector<PointCourior> *tabu_route)
{
	list<pair<int, double> > route_dis_list;
	for (int i = 0; i < COURIOR_NUM; i++) {
		PointOrder p = initial.courior[i].path->head;
		if (p != NULL) {
			route_dis_list.push_back(make_pair(i, initial.courior[i].get_dis()));
		}
	}
	route_dis_list.sort(route_sort_descend);
	int oprt_route = round(route_dis_list.size()*Alns::tabu_size_rate);
	int oprt_order = round((Alns::oto_num + Alns::eco_num)*Alns::tabu_size_rate);
	for (int i = 0, order_count = 0; i < oprt_route || order_count < oprt_order; i++) {
		tabu_route->push_back(&initial.courior[route_dis_list.front().first]);
		PointOrder p = initial.courior[route_dis_list.front().first].path->head;
		while (p != NULL) {
			if (p->type)
				order_count++;
			p = p->next;
		}
		route_dis_list.pop_front();
	}
}

void Alns::load_selection(vector<PointCourior> *tabu_route)
{
	list<pair<int, double> > route_load_list;
	for (int i = 0; i < COURIOR_NUM; i++) {
		PointOrder p = initial.courior[i].path->head;
		if (p != NULL) {
			route_load_list.push_back(make_pair(i, initial.courior[i].get_load()));
		}
	}
	route_load_list.sort(route_sort_descend);
	int oprt_route = round(route_load_list.size()*Alns::tabu_size_rate);
	int oprt_order = round((Alns::oto_num + Alns::eco_num)*Alns::tabu_size_rate);
	for (int i = 0, order_count = 0; i < oprt_route || order_count < oprt_order; i++) {
		tabu_route->push_back(&initial.courior[route_load_list.front().first]);
		PointOrder p = initial.courior[route_load_list.front().first].path->head;
		while (p != NULL) {
			if (p->type)
				order_count++;
			p = p->next;
		}
		route_load_list.pop_front();
	}
}

void Alns::get_depot_lb()
{
	map<string, vector<string> > depot_order;
	for (int i = 0;i < COURIOR_NUM;i++) {
		PointOrder p = initial.courior[i].path->head;
		while (p != NULL) {
			if (p->type && Order::check_order(p->package->id)) {
				depot_order[p->package->depot_id].push_back(p->package->id);
			}
			p = p->next;
		}
	}
	for (map<string, vector<string> >::iterator iter = depot_order.begin();iter != depot_order.end();++iter) {
		Position depot_pos = FileIO::netpoint[Util::string_to_int(iter->first)];
		int total_weight = 0;
		vector<string>::iterator depot_order = iter->second.begin();
		double depot_min_arc = MAX_DS, order_min_arc = MAX_DS;
		for (vector<string>::iterator depot_order = iter->second.begin();depot_order != iter->second.end();++depot_order) {
			Position order_pos = FileIO::order_map[*depot_order]->dest;
			total_weight += FileIO::order_map[*depot_order]->weight;
			double arc = Util::get_distance(depot_pos, order_pos);
			if (depot_min_arc > arc)
				depot_min_arc = arc;
		}
		for (vector<string>::iterator order1 = iter->second.begin();order1 != iter->second.end();++order1) {
			Position order1_pos = FileIO::order_map[*order1]->dest;
			for (vector<string>::iterator order2 = iter->second.begin();order2 != iter->second.end();++order2) {
				if (*order1 == *order2)
					continue;
				Position order2_pos = FileIO::order_map[*order2]->dest;
				double arc = Util::get_distance(order1_pos, order2_pos);
				if (order_min_arc > arc)
					order_min_arc = arc;
			}
		}
		double trip = ceil(double(total_weight) / CAPACITY);
		int order_num = iter->second.size();
		double lb = trip * depot_min_arc + (order_num - trip)*order_min_arc;
		depot_lb[iter->first] = lb;
	}
}

double Alns::scale_rate_adjust(int scale)
{
	if (MOVE_SIZE_MAX*double(scale) > 500)
		return 500 / double(scale);
	else
		return MOVE_SIZE_MAX;
}

bool Alns::route_sort_ascend(const pair<int, double>& a, const pair<int, double>& b)
{
	return a.second < b.second;
}

bool Alns::route_sort_descend(const pair<int, double>& a, const pair<int, double>& b)
{
	return a.second > b.second;
}

bool Alns::pair_sort_ascend(const pair<string, double>& a, const pair<string, double>& b)
{
	return a.second < b.second;
}

bool Alns::pair_sort_descend(const pair<string, double>& a, const pair<string, double>& b)
{
	return a.second > b.second;
}

int Alns::noise(){
	double e = rand() / float(RAND_MAX) * 2 - 1, u = 0.2;
	int d = 50;
	return u*d*e;
} 

int Alns::roulette_wheel(int flag) {
	float prob = rand() / float(RAND_MAX), sum = 0;
	if (flag == 1){
		for (int i = 0; i < DEL_NUM; i++){
			sum += del_operator[i].prob;
			if (sum >= prob) {
				return i;
			}
		}
		return rand() % DEL_NUM;
	}
	else if (flag == 0) {
		for (int i = 0; i < INS_NUM; i++){
			sum += ins_operator[i].prob;
			if (sum >= prob) {
				return i;
			}
		}
		return rand() % INS_NUM;
	}
	else {
		for (int i = 0; i < TABU_OPRT; i++) {
			sum += tabu_operator[i].prob;
			if (sum >= prob) {
				return i;
			}
		}
		return rand() % TABU_OPRT;
	}
}

void Alns::update_prob(int del_num, int ins_num) {
	double total_delete_prob = 0, total_insert_prob = 0, total_route_prob = 0;
	for (int i = 0; i < DEL_NUM; i++) {
		del_operator[i].prob = del_operator[i].prob *(1 - roulette) + roulette * del_operator[i].score / del_operator[i].times;
		total_delete_prob += del_operator[i].prob;
	}
	for (int i = 0; i < DEL_NUM; i++) {
		del_operator[i].prob = del_operator[i].prob / total_delete_prob;
	}
	for (int i = 0; i < INS_NUM; i++) {
		ins_operator[i].prob = ins_operator[i].prob *(1 - roulette) + roulette * ins_operator[i].score / ins_operator[i].times;
		total_insert_prob += ins_operator[i].prob;
	}
	for (int i = 0; i < INS_NUM; i++) {
		ins_operator[i].prob = ins_operator[i].prob / total_insert_prob;
	}
}

void Alns::tabu_oprt_update()
{
	double total_prob = 0;
	for (int i = 0; i < TABU_OPRT; i++) {
		tabu_operator[i].prob = tabu_operator[i].prob *(1 - roulette) + roulette * tabu_operator[i].score / tabu_operator[i].times;
		total_prob += tabu_operator[i].prob;
	}
	for (int i = 0; i < TABU_OPRT; i++) {
		tabu_operator[i].prob = tabu_operator[i].prob / total_prob;
	}
}

void Alns::solution_copy(Solution *dest, Solution *src){
	dest->vehicleNumber = src->vehicleNumber;
	dest->cost = src->cost;
	dest->travelTime = src->travelTime;
	dest->waitTime = src->waitTime;
	dest->penalty = src->penalty;
	dest->idleTime = src->idleTime;
	for (int i = 0; i < COURIOR_NUM; i++){
		dest->courior[i].delete_path();
		PointOrder p = src->courior[i].path->head;
		PointOrder dest_head = NULL, pre = NULL;
		while (p != NULL){
			PointOrder order = new Order(p->package, 0);
			dest->courior[i].order_info_copy(order, p);
			if (dest_head == NULL)
				dest_head = order;
			else
				pre->next = order;
			order->prior = pre;
			pre = order;
			p = p->next;
		}
		dest->courior[i].path->head = dest_head;
		dest->courior[i].path->tail = pre;
	}
}
