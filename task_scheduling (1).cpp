#include <iostream>
#include <algorithm>
#include <stack>
#include <vector>
#include <list>
#include <iterator>
#include <chrono>
#include <iomanip>
#include <climits>
using namespace std;

/*
Declaring Global Variables
*/
    int num_cores = 3;
    int num_tasks = 10;
    // int num_tasks = 20;
    int start_time[40],initial_finish_time,maximum_finish_time;
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2}};
    int tasks_map[][40]={{0,1,1,1,1,1,0,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,1,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0}}; 
    int is_cloud[40] = { 0 }; //If true then 1;else 0;
    int priority[40] = { 0 }, nodes_priority_order[40] = { 0 }, computation_cost[40] = { 0 }, core_occupancy_timeline[4] = { 0 }, task_execution_location[40] = { 0 }, ready_time_local[40] = { 0 }, ready_time_cloud_compution[40] = { 0 }, readytime_wireless_sending[40] = { 0 }, finishtime_wireless_sending[40] = { 0 }, finishtime_wireless_receiving[40] = { 0 }, finishtime_local[40] = { 0 }, finish_time[40] = { 0 };
    vector<vector<int>> core_task_exec_lst(4);
    int send_time = 3, compute_time = 1, receive_time = 1;
    int total_runtime_cloud = send_time + compute_time + receive_time; // =5
    int energy_local_map[40][3];
    int cores_power_map[] = {1,2,4};
    float ps = 0.5,initial_total_energy;
    float energy_cloud = ps * send_time;
/*
End of Declaring Global Variables
*/


void primary_assignment(int core_execution_time_map[][3], int num_tasks){
    int runtime_local_min[40] = {total_runtime_cloud};
    for(int i=0; i<num_tasks; i++){
        for(int j=0; j<num_cores; j++){
            if(runtime_local_min[i]>core_execution_time_map[i][j]){
                runtime_local_min[i] = core_execution_time_map[i][j];
            }
        }
    }
    //classify tasks: value of is_cloud[i] 0:local 1:cloud
    for(int i=0; i<num_tasks; i++){
        if(runtime_local_min[i] > total_runtime_cloud){
            is_cloud[i] = 1;
        }
        else
            is_cloud[i] = 0;
    }
}

void task_prioritizing(int core_execution_time_map[][3], int tasks_map[][40], int num_tasks){
    for(int i=0; i<num_tasks; i++){
        if(is_cloud[i] == 1){
            computation_cost[i] = total_runtime_cloud;
        }
        else{
            int sum = 0;
            for(int j=0; j<num_cores; j++){
                sum += core_execution_time_map[i][j];
            }
            computation_cost[i] = sum/num_cores;
        }
    }
    priority[num_tasks-1] = computation_cost[num_tasks-1];
    for(int i=num_tasks-1; i>=0; i--){
        int max_j = 0;
        for(int j=num_tasks-1; j>=0; j--){
            if(tasks_map[i][j] == 1 && priority[j] > max_j){
                max_j = priority[j];
            }
        }
        priority[i] = computation_cost[i] + max_j;
    }
    vector<pair<int,int>> vect;
    for (int i=0; i<num_tasks; i++){
        vect.push_back(make_pair(priority[i],i));
    }
    sort(vect.begin(), vect.end());
    for(int i=0; i<num_tasks; i++){
        nodes_priority_order[i] = vect[i].second;
    }
}

void execution_selection(int core_execution_time_map[][3],int nodes_priority_order[], int tasks_map[][40], int is_cloud[], int num_tasks, vector<vector<int>> core_task_exec_lst){
    int f_i = nodes_priority_order[num_tasks-1];
    ready_time_local[f_i] = 0;
    readytime_wireless_sending[f_i] = 0;
    finishtime_wireless_sending[f_i] = readytime_wireless_sending[f_i] + send_time;
    ready_time_cloud_compution[f_i] = finishtime_wireless_sending[f_i];
    if(is_cloud[f_i] == 1){
        finishtime_wireless_receiving[f_i] = ready_time_cloud_compution[f_i] + compute_time + receive_time;
        finishtime_local[f_i] = 0;
        finish_time[f_i] = finishtime_wireless_receiving[f_i];
        core_occupancy_timeline[3] = finish_time[f_i];
        core_task_exec_lst[0].push_back(f_i);
        task_execution_location[f_i] = 3;
    }
    else{
        int runtime_local_min = INT_MAX;
        int index;
        for(int i=0; i<num_cores; i++){
            if(core_execution_time_map[f_i][i]<runtime_local_min){
                runtime_local_min = core_execution_time_map[f_i][i];
                index = i;
            }
        }
        finishtime_local[f_i] = ready_time_local[f_i] + runtime_local_min;
        finishtime_wireless_receiving[f_i] = 0;
        finish_time[f_i] = finishtime_local[f_i];
        core_occupancy_timeline[index] = finish_time[f_i];
        core_task_exec_lst[index+1].push_back(f_i);
        task_execution_location[f_i] = index;
    }
    for(int a=num_tasks-2; a>=0; a--){
        int i = nodes_priority_order[a];
        int max_j_l = 0;
        for(int j=0; j<num_tasks; j++){
            if(tasks_map[j][i] == 1 && max_j_l < max(finishtime_local[j],finishtime_wireless_receiving[j])){
                max_j_l = max(finishtime_local[j],finishtime_wireless_receiving[j]);
            }
        }
        ready_time_local[i] = max_j_l;
        int max_j_ws = 0;
        for(int j=0; j<num_tasks; j++){
            if(tasks_map[j][i] == 1 && max_j_ws < max(finishtime_local[j],finishtime_wireless_sending[j])){
                max_j_ws = max(finishtime_local[j],finishtime_wireless_sending[j]);
            }
        }
        readytime_wireless_sending[i] = max_j_ws;
        finishtime_wireless_sending[i] = max(core_occupancy_timeline[3],readytime_wireless_sending[i]) + send_time;
        int max_j_c = 0;
        for(int j=0; j<num_tasks; j++){
            if(tasks_map[j][i] == 1 && max_j_c < finishtime_wireless_receiving[j]-receive_time){
                max_j_c = finishtime_wireless_receiving[j]-receive_time;
            }
        }
        ready_time_cloud_compution[i] = max(finishtime_wireless_sending[i],max_j_c);
        if(is_cloud[i] == 1){
            finishtime_wireless_receiving[i] = ready_time_cloud_compution[i] + compute_time + receive_time;
            finish_time[i] = finishtime_wireless_receiving[i];
            finishtime_local[i] = 0;
            core_occupancy_timeline[3] = finishtime_wireless_sending[i];
            core_task_exec_lst[0].push_back(i);
            task_execution_location[i] = 3;
        }
        else{
            int ready_time, index;
            int f = INT_MAX;
            for(int j=0; j<num_cores; j++){
                ready_time = max(ready_time_local[i],core_occupancy_timeline[j]);
                if(f > ready_time + core_execution_time_map[i][j]){
                    f = ready_time + core_execution_time_map[i][j];
                    index = j;
                }
            }
            ready_time_local[i] = f - core_execution_time_map[i][index];
            finishtime_local[i] = f;
            finishtime_wireless_receiving[i] = ready_time_cloud_compution[i] + compute_time + receive_time;
            if(finishtime_local[i] <= finishtime_wireless_receiving[i]){
                finish_time[i] = finishtime_local[i];
                finishtime_wireless_receiving[i] = 0;
                readytime_wireless_sending[i] = 0;
                core_occupancy_timeline[index] = finish_time[i];
                core_task_exec_lst[index+1].push_back(i);
                task_execution_location[i] = index;
            }
            else{
                finish_time[i] = finishtime_wireless_receiving[i];
                finishtime_local[i] = 0;
                core_occupancy_timeline[3] = finish_time[i];
                core_task_exec_lst[0].push_back(i);
                task_execution_location[i] = 3;
            }
        }
    }
}



void task_migration( vector<vector<int>> core_task_exec_lst, int core_execution_time_map[][3], int tasks_map[][40],int task_execution_location[],int maximum_finish_time, int initial_finish_time, float initial_total_energy, int num_tasks, int start_time[], int end_time[],float energy_cloud){
    int flag = 0;
    while(flag == 0){
        int new_execution_location[40], new_start_time[40], new_finish_time[40], new_task = 0, new_core = 0, new_index1 = 0, new_index2 = 0, new_time = initial_finish_time, l1 =0, l2 = 0,powerbytime_ratio = 0;
        float new_energy = initial_total_energy;
        for(int i=0; i<num_tasks; i++){
            for(int j=0; j<num_cores+1; j++){
                int task_execution_location1[40], core_occupancy_timeline1[4], ready_time[40], ready_time1[40], finish_time[40], finish_time1[40], processed[40];
                vector<vector<int>> core_task_exec_lst_new(4);
                int index1, index2 = 0;
                for(int i=0; i<num_tasks; i++){
                    ready_time[i] = 0;
                    finish_time[i] = 0;
                    task_execution_location1[i] = task_execution_location[i];
                    finish_time1[i] = end_time[i];
                    ready_time1[i] = start_time[i];
                    processed[i] = 0;
                }
                //replicate core_task_exec_lst_new same as original scheduling
                for(int a=0; a<core_task_exec_lst.size(); a++){
                    core_occupancy_timeline1[a] = 0;
                    for(int b=0; b<core_task_exec_lst[a].size(); b++){
                        core_task_exec_lst_new[a].push_back(core_task_exec_lst[a][b]);
                    }
                }
                // cout << "test1\n";
                int current_core = task_execution_location[i];
                for(int a=0; a<core_task_exec_lst_new[current_core].size(); a++){
                    if(core_task_exec_lst_new[current_core][a] == i){
                        index1 = a;
                    }
                }
                core_task_exec_lst_new[current_core].erase(core_task_exec_lst_new[current_core].begin()+index1);
                ready_time[i] = 0;
                for(int a=0; a<num_tasks; a++){
                    if(tasks_map[a][i] == 1 && ready_time[i] < finish_time1[a]){
                        ready_time[i] = finish_time1[a];
                    }
                }

                task_execution_location1[i] = j;
                if(core_task_exec_lst_new[j].size() == 0){
                    index2 = 0;
                }
                else if(core_task_exec_lst_new[j].size() == 1){
                    if(ready_time1[core_task_exec_lst_new[j][0]] > ready_time[i]){
                        index2 = 0;
                    }
                    else{
                        index2 = 1;
                    }
                }
                else{
                    if(ready_time1[core_task_exec_lst_new[j][0]] > ready_time[i]){
                        index2 = 0;
                    }
                    else if(ready_time1[core_task_exec_lst_new[j][core_task_exec_lst_new[j].size()-1]] <= ready_time[i]){
                        index2 = core_task_exec_lst_new[j].size();
                    }
                    else{
                        for(int b=0; b<core_task_exec_lst_new[j].size()-1; b++){
                            if(ready_time[i]>=ready_time1[core_task_exec_lst_new[j][b]] && ready_time[i]<=ready_time1[core_task_exec_lst_new[j][b+1]]){
                                index2 = b+1;
                            }
                        }
                    }
                }
                // cout << "test2\n";
                core_task_exec_lst_new[j].insert(core_task_exec_lst_new[j].begin()+index2,i);
                int ready1[40] = {0}, ready2[40] = {0};
                for(int a=0; a<num_tasks; a++){
                    for(int b=0; b<num_tasks; b++){
                        if(tasks_map[a][b] == 1){
                            ready1[b] += 1;
                        }
                    }
                    ready2[a] = 1;
                }
                for(int a=0; a<4; a++){
                    if(core_task_exec_lst_new[a].size()>0){
                        ready2[core_task_exec_lst_new[a][0]] = 0;
                    }
                }

                //intializing stack
                stack<int> s;
                for(int a=0; a<num_tasks; a++){
                    if(ready1[a] == 0 && ready2[a] == 0 && processed[a] == 0){
                        s.push(a);
                        processed[a] = 1;
                    }
                    // else{
                    //     continue;
                    // }
                }
                int current1 = s.top();
                s.pop();
                // cout << "test2\n";
                ready_time[current1] = 0;
                if(task_execution_location1[current1] == 3){
                    ready_time[current1] = max(core_occupancy_timeline1[task_execution_location1[current1]],ready_time[current1]);
                    finish_time[current1] = ready_time[current1] + send_time + compute_time + receive_time;
                    core_occupancy_timeline1[task_execution_location1[current1]] = ready_time[current1] + send_time;
                }
                else{
                    ready_time[current1] = max(core_occupancy_timeline1[task_execution_location1[current1]],ready_time[current1]);
                    finish_time[current1] = ready_time[current1] + core_execution_time_map[current1][task_execution_location1[current1]];
                    core_occupancy_timeline1[task_execution_location1[current1]] = finish_time[current1];
                }
                // cout << task_execution_location1[current1] << " " << core_occupancy_timeline1[task_execution_location1[current1]] << "\n";;
                //update ready1 and ready2
                // cout << "test3\n";
                for(int a=0; a<num_tasks; a++){
                    if(tasks_map[current1][a] == 1){
                        ready1[a] -= 1;
                    }
                }
                ready2[current1] = 1;
                if(core_task_exec_lst_new[task_execution_location1[current1]].size()>1){
                    for(int a=1; a<core_task_exec_lst_new[task_execution_location1[current1]].size(); a++){
                        if(core_task_exec_lst_new[task_execution_location1[current1]][a-1] == current1){
                            ready2[core_task_exec_lst_new[task_execution_location1[current1]][a]] = 0;
                            //cout<<core_task_exec_lst_new[task_execution_location1[current1]][a]<<" "<<i<<" "<<j<<"\n";;
                        }
                    }
                }
                for(int a=0; a<num_tasks; a++){
                    if(ready1[a] == 0 && ready2[a] == 0 && processed[a] == 0){
                        s.push(a);
                        processed[a] = 1;
                    }
                }

                while(s.size() != 0){
                    int current = s.top();
                    s.pop();

                    ready_time[current] = 0;

                    if(task_execution_location1[current] == 3){
                        int max_ws = 0;
                        for(int a=0; a<num_tasks; a++){
                            if(tasks_map[a][current] == 1 && max_ws < ready_time[a]){
                                max_ws = ready_time[a];
                            }
                        }
                        ready_time[current] = max_ws;
                    }
                    else{
                        int max_wl = 0;
                        for(int a=0; a<num_tasks; a++){
                            if(tasks_map[a][current] == 1 && max_wl < finish_time[a]){
                                max_wl = finish_time[a];
                            }
                        }
                        ready_time[current] = max_wl;
                    }

                    if(task_execution_location1[current] == 3){
                        ready_time[current] = max(core_occupancy_timeline1[task_execution_location1[current]],ready_time[current]);
                        finish_time[current] = ready_time[current] + 5;
                        core_occupancy_timeline1[task_execution_location1[current]] = ready_time[current] + 3;
                    }
                    else{
                        ready_time[current] = max(core_occupancy_timeline1[task_execution_location1[current]],ready_time[current]);
                        finish_time[current] = ready_time[current] + core_execution_time_map[current][task_execution_location1[current]];
                        core_occupancy_timeline1[task_execution_location1[current]] = finish_time[current];
                    }
                    // cout << current << "T - " << task_execution_location1[current] << " - (" << ready_time[current] << "," << finish_time[current] << "); ";
                    // cout << ready_time[current] << " " << finish_time[current] << "; ";
                    for(int a=0; a<num_tasks; a++){
                        if(tasks_map[current][a] == 1){
                            ready1[a] -= 1;
                        }
                    }
                    ready2[current] = 1;
                    if(core_task_exec_lst_new[task_execution_location1[current]].size()>1){
                        for(int a=1; a<core_task_exec_lst_new[task_execution_location1[current]].size(); a++){
                            if(core_task_exec_lst_new[task_execution_location1[current]][a-1] == current){
                                ready2[core_task_exec_lst_new[task_execution_location1[current]][a]] = 0;
                            }
                        }
                    }
                    for(int a=0; a<num_tasks; a++){
                        if(ready1[a] == 0 && ready2[a] == 0 && processed[a] == 0){
                            s.push(a);
                            processed[a] = 1;
                        }
                    }
                }
                // cout << "test4\n";
                int current_t = finish_time[num_tasks-1];
                // cout << current_t << "; ";
                float current_e = 0;
                for(int a=0; a<num_tasks; a++){
                    if(task_execution_location1[a] == 3){
                        current_e += energy_cloud;
                        // cout << energy_cloud << "\n";
                        // cout << a << ":" << current_e << " ";
                    }
                    else{
                        current_e += energy_local_map[a][task_execution_location1[a]];
                        // cout << a << ":" << current_e << " ";
                    }
                }
                if(current_t!=0 && current_t <= initial_finish_time && current_e < new_energy){
                    l1 = 1;
                    new_task = i;
                    new_core = j;
                    new_index1 = index1;
                    new_index2 = index2;
                    new_time = current_t;
                    new_energy = current_e;
                    for(int a=0; a<num_tasks; a++){
                        new_execution_location[a] = task_execution_location1[a];
                        new_start_time[a] = ready_time[a];
                        new_finish_time[a] = finish_time[a];
                     }
                }
                if(current_t!=0 && current_t > initial_finish_time && current_t <= maximum_finish_time && l1 == 0 && current_e < initial_total_energy && powerbytime_ratio < double((initial_total_energy - current_e) / (current_t - initial_finish_time))){
                    powerbytime_ratio = double((initial_total_energy - current_e) / (current_t - initial_finish_time));
                    l2 = 1;
                    new_task = i;
                    new_core = j;
                    new_index1 = index1;
                    new_index2 = index2;
                    new_time = current_t;
                    new_energy = current_e;
                    for(int a=0; a<num_tasks; a++){
                        new_execution_location[a] = task_execution_location1[a];
                        new_start_time[a] = ready_time[a];
                        new_finish_time[a] = finish_time[a];
                    }
                }
            }
        }
        // cout << "l1 , l2 " << l1 <<" " << l2 << "; ";
        if(l1 != 1 && l2 != 1){
            flag = 1;
        }
        else{
            core_task_exec_lst[task_execution_location[new_task]].erase(core_task_exec_lst[task_execution_location[new_task]].begin()+new_index1);
            core_task_exec_lst[new_core].insert(core_task_exec_lst[new_core].begin()+new_index2,new_task);
            initial_finish_time = new_time;
            initial_total_energy = new_energy;
            for(int a=0; a<num_tasks; a++){
                task_execution_location[a] = new_execution_location[a];
                start_time[a] = new_start_time[a];
                end_time[a] = new_finish_time[a];
            }
            if(l1 != 1 && l2 != 1){
                flag = 1;
            }
            cout<<"Current Operation: Insert Task "<<new_task+1<<" to core_occupancy_timeline "<<new_core+1<<"\n";
            cout<<"Current Completion Time: "<<initial_finish_time<< "   Current Energy Consumption: "<<initial_total_energy<<"\n";
        }
    }
    cout<<"\n";
    cout<<"Best Asssignment"<<"\n";
    cout<<endl;
    for(int i=0; i<core_task_exec_lst.size(); i++){
        if(i == 3){
            cout<<"Cloud: ";
        }
        else{
            cout<<"core_occupancy_timeline "<<i+1<<": ";
        }
        for(int j=0; j<core_task_exec_lst[i].size(); j++){
            cout<<start_time[core_task_exec_lst[i][j]]<<"<--Task"<<core_task_exec_lst[i][j]+1<<"-->"<<end_time[core_task_exec_lst[i][j]]<<"  :  ";
        }
        cout<<"\n";
    }
    cout<<"Best Energy Consumption: "<<initial_total_energy<<"   Best Completion Time: "<<initial_finish_time<<"\n";

}







// void initial_assignment_stats(task_execution_location,core_task_exec_lst)
void initial_assignment_stats()
{

    for(int i=0; i<4; i++){
        for(int j=0; j<num_tasks; j++){
            if(task_execution_location[j] == i){
                core_task_exec_lst[i].push_back(j);
            }
        }
    }
    for(int i=0; i<num_tasks; i++){
        if(task_execution_location[i] == 3){
            initial_total_energy += energy_cloud;
        }
        else{
            initial_total_energy += energy_local_map[i][task_execution_location[i]];
        }
    }
    for(int i=0; i<num_tasks; i++){
        start_time[i] = max(ready_time_local[i],readytime_wireless_sending[i]);
    }

    initial_finish_time = finish_time[num_tasks-1];
    maximum_finish_time = initial_finish_time*1.5;
    cout<<"Initial Scheduling (Format: Start Time - Task Number - Finish Time)"<<"\n";
    for(int i=0; i<core_task_exec_lst.size(); i++){
        if(i == 3){
            cout<<"Cloud: ";
        }
        else{
            cout<<"core_occupancy_timeline"<<i+1<<": ";
        }
        for(int j=0; j<core_task_exec_lst[i].size(); j++){
            cout<<start_time[core_task_exec_lst[i][j]]<<"<--Task"<<core_task_exec_lst[i][j]+1<<"-->"<<finish_time[core_task_exec_lst[i][j]]<<"  :  ";
        }
        cout<<"\n";
    }
    cout<<"Initial Energy Consumption: "<<initial_total_energy<<"   Initial Completion Time: "<<initial_finish_time<<"\n";
    cout<<"\n";
}

int main(int argc, char *argv[])
{
    for(int i=0; i<num_tasks; i++){
      for(int j=0; j<num_cores; j++){
          energy_local_map[i][j] = cores_power_map[j] * core_execution_time_map[i][j];
      }
    }


/*INPUT FOR TEST CASE 1*/
    int num_tasks = 10;
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2}};
    int tasks_map[][40]={{0,1,1,1,1,1,0,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,1,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0}}; 

/*INPUT FOR TEST CASE 2*/
    
/*    int num_tasks = 10;  
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2}};
    int tasks_map[][40]={{0,1,1,1,0,0,0,0,0,0},
                         {0,0,0,0,1,0,0,0,0,0},
                         {0,0,0,0,1,1,0,0,0,0},
                         {0,0,0,0,0,0,1,0,0,0},
                         {0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0}}; */
/*INPUT FOR TEST CASE 3*/

/*    int num_tasks = 20;  
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2},{7,5,2},{8,5,2},{5,4,3},{7,5,4},{8,7,4},{9,7,5},{8,5,4},{7,4,3},{6,3,2},{5,3,2}};
    int tasks_map[][40]={{0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}; */
/*INPUT FOR TEST CASE 4*/
/*    int num_tasks = 20;  
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2},{7,5,2},{8,5,2},{5,4,3},{7,5,4},{8,7,4},{9,7,5},{8,5,4},{7,4,3},{6,3,2},{5,3,2}};
    int tasks_map[][40]={{0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}; */

/*INPUT FOR TEST CASE 5*/
/*    int num_tasks = 20;  
    int core_execution_time_map[][3]={{9,7,5},{8,6,5},{6,5,4},{7,5,3},{5,4,2},{7,6,4},{8,5,3},{6,4,2},{5,3,2},{7,4,2},{7,5,2},{8,5,2},{5,4,3},{7,5,4},{8,7,4},{9,7,5},{8,5,4},{7,4,3},{6,3,2},{5,3,2}};
    int tasks_map[][40]={{0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
                         {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
                         */

    primary_assignment(core_execution_time_map,num_tasks);
    task_prioritizing(core_execution_time_map,tasks_map,num_tasks);

    execution_selection(core_execution_time_map,nodes_priority_order,tasks_map,is_cloud,num_tasks,core_task_exec_lst);
    initial_assignment_stats();
    task_migration(core_task_exec_lst,core_execution_time_map,tasks_map,task_execution_location,maximum_finish_time,initial_finish_time,initial_total_energy,num_tasks,start_time,finish_time,energy_cloud);

    /*for(int i=0; i<num_tasks; i++){
        cout<<task_execution_location[i]<<" ";
    }*/
    return 0;
}
