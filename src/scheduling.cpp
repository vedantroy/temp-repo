#include <scheduling.h>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <string>
#include <sstream>

using namespace std;

pqueue_arrival read_workload(string filename) {
  pqueue_arrival workload;

  std::ifstream infile;
  infile.open(filename);

  vector <string> lines;
  std::string line;
  while(getline(infile, line)) {
      lines.push_back(line);
  }

  for (auto line : lines) {
      stringstream ss;
      ss << line;
      int start_time, duration;
      ss >> start_time;
      ss >> duration;

      Process p;
      p.arrival = start_time;
      p.duration = duration;
      p.first_run = -1;
      p.completion = p.arrival;
      workload.push(p);
  }

  return workload;
}

void show_workload(pqueue_arrival workload) {
  pqueue_arrival xs = workload;
  cout << "Workload:" << endl;
  while (!xs.empty()) {
    Process p = xs.top();
    cout << '\t' << p.arrival << ' ' << p.duration << endl;
    xs.pop();
  }
}

void show_processes(list<Process> processes) {
  list<Process> xs = processes;
  cerr << "Processes:" << endl;
  while (!xs.empty()) {
    Process p = xs.front();
    cerr << "\tarrival=" << p.arrival << ", duration=" << p.duration
         << ", first_run=" << p.first_run << ", completion=" << p.completion
         << endl;
    xs.pop_front();
  }
}

list<Process> fifo(pqueue_arrival workload) {
  list<Process> complete;
  int cur_time = 0;
  while (!workload.empty()) {
      Process top = workload.top();
      workload.pop();
      cur_time = top.arrival > cur_time ? top.arrival : cur_time;
      top.first_run = cur_time;
      top.completion = cur_time + top.duration;
      cur_time += top.duration;
      complete.push_back(top);
  }
  return complete;
}

list<Process> sjf(pqueue_arrival workload) {
  list<Process> complete;
  pqueue_duration sched;
  int cur_time = 0;

  while (true) {
      if (sched.size() == 0 && workload.size() == 0) {
          // we're done
          break;
      }

      while (true) {
          if (workload.size() == 0) break;
          Process top = workload.top();
          if (top.arrival <= cur_time) {
              workload.pop();
              sched.push(top);
          } else break;
      }

      if (sched.size() == 0) {
          Process top = workload.top();
          workload.pop();
          sched.push(top);
          continue;
      }

      Process top = sched.top();
      sched.pop();
      cur_time = top.arrival > cur_time ? top.arrival : cur_time;
      top.first_run = cur_time;
      top.completion = cur_time + top.duration;
      complete.push_back(top);
      cur_time = top.completion;
  }
  return complete;
}

list<Process> stcf(pqueue_arrival workload) {
  list<Process> complete;
  pqueue_duration sched;

  int cur_time = 0;

  int state_empty_sched = 0;
  int state_run_sched = 1;
  int state_get_newest_arrivals = 2;
  int cur_state = state_empty_sched;

  while (true) {
      if (cur_state == state_empty_sched) {
          if (workload.size() == 0) break;
          cur_state = state_get_newest_arrivals;
      } else if (cur_state == state_get_newest_arrivals) {
          auto peek = workload.top();
          int min_arrival = peek.arrival;
          while (workload.size() > 0) {
              auto proc = workload.top();
              if (proc.arrival == min_arrival) {
                  workload.pop();
                  sched.push(proc);
              } else break;
          }
          cur_state = state_run_sched;
      } else if (cur_state == state_run_sched) {
          if (sched.size() == 0) {
              cur_state = state_empty_sched;
              continue;
          }

          int next_arrival = -1;
          if (workload.size() > 0) {
              auto top = workload.top();
              next_arrival = top.arrival;
          }

          auto top = sched.top();
          sched.pop();

          if (top.first_run == -1) {
              top.first_run = cur_time > top.arrival ? cur_time : top.arrival;
              cur_time = top.first_run;
          }

          if (next_arrival == -1 || cur_time + top.duration < next_arrival) {
              top.completion = cur_time + top.duration;
              top.duration = 0;
              cur_time = top.completion;
              complete.push_back(top);
              continue;
          }

          int time_till_interrupt = next_arrival - top.completion;
          int time_left = top.duration - time_till_interrupt;
          top.duration = time_left;
          top.completion = next_arrival;

          cur_time = next_arrival;
          cur_state = state_get_newest_arrivals;
          sched.push(top);
      }
  }
  return complete;
}

list<Process> rr(pqueue_arrival workload) {
  list<Process> complete;
  vector<Process> queue;

  int queue_idx = 0;
  int cur_state = 0;
  int cur_time = 0;

  int state_end_of_queue = 0;
  int state_run_time_slice = 1;

  while (true) {
      if (cur_state == state_end_of_queue) {
          bool queue_empty = queue.size() == 0;
          bool workload_empty = workload.size() == 0;
          if (workload_empty && queue_empty) {
              break;
          } else if (!workload_empty) {
            auto peek = workload.top();
            int min_arrival = peek.arrival;
            if (min_arrival <= cur_time || queue_empty) {
                while (workload.size() > 0) {
                    auto proc = workload.top();
                    if (proc.arrival == min_arrival) {
                        workload.pop();
                        queue.push_back(proc);
                    } else break;
                }
                cur_state = state_run_time_slice;
            }
            if (queue_empty) {
                cur_time = min_arrival;
            }
          }
          queue_idx = 0;
          cur_state = state_run_time_slice;
      } else if (cur_state == state_run_time_slice) {
          Process* p = &queue[queue_idx];
          if (p->first_run == -1) {
              p->first_run = cur_time;
          }
          if (p->duration == 1) {
              p->completion = cur_time + 1;
              complete.push_back(*p);
              auto it = queue.begin();
              advance(it, queue_idx);
              queue.erase(it);
          } else {
              p->duration -= 1;
              queue_idx += 1;
              if (queue_idx == queue.size()) {
                  cur_state = state_end_of_queue;
              }
          }
          cur_time += 1;
      }
  }
  return complete;
}

float avg_turnaround(list<Process> processes) {
  vector<int> times;
  for (auto process: processes) {
      int ttime = process.completion - process.arrival;
      times.push_back(ttime);
  }

  int sum = 0;
  for (int i : times) {
      sum += i;
  }
  return (float)sum / (float)times.size();
}

float avg_response(list<Process> processes) {
  vector<int> times;
  for (auto process: processes) {
      int rtime = process.first_run - process.arrival;
      times.push_back(rtime);
  }

  int sum = 0;
  for (int i : times) {
      sum += i;
  }
  return (float)sum / (float)times.size();
}

void show_metrics(list<Process> processes) {
  float avg_t = avg_turnaround(processes);
  float avg_r = avg_response(processes);
  show_processes(processes);
  cout << '\n';
  cout << "Average Turnaround Time: " << avg_t << endl;
  cout << "Average Response Time:   " << avg_r << endl;
}
