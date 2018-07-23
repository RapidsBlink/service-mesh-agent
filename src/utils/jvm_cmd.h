#pragma once

//return nspid
int init_jvm_comm_socket(int pid);

int jvm_run_cmd(int nspid, int argc, char** argv);