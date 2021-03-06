#include <sys/types.h>
#include <sys/commons.h>
#include <sys/vfs.h>
#include <sys/dirent_s.h>
#include <sys/kern_process.h>
#include <sys/kprintf.h>
#include <sys/kmalloc.h>
#include <sys/memset.h>
#include <sys/memcpy.h>
#include <sys/kstring.h>
#include <sys/page_table.h>


#define PAGE_SIZE  4096
extern uint64_t MAP_UB; 
extern task_struct* s_cur_run_task;
extern uint64_t KB;
extern v_file_node* tarfs_mount_node;
extern void map_process_address_space();
extern void page_fault_handler();
uint64_t curr_available_file_des_num = 3;


 dir_info * find_dir(uint64_t des) {
	if (s_cur_run_task == NULL) {
         kprintf("\nKernel Panic : No current running process");
          return NULL;
      }
  
      dir_info * trav = s_cur_run_task->file_root;
      if (trav == NULL) {
          return NULL;
      }
      while (trav != NULL) {
          if (trav->des == des) {
              return trav;
          }
          trav = trav->next;
      }
      return NULL;
 }

 dir_info * find_dir_by_name(const char * name) {
	if (s_cur_run_task == NULL) {
         kprintf("\nKernel Panic : No current running process");
          return NULL;
      }   
  
      dir_info * trav = s_cur_run_task->file_root;
      if (trav == NULL) {
          return NULL;
      }   

	  while (trav != NULL) {
		  if (trav->v_node != NULL) {
              if (kstrcmp(trav->v_node->v_name, name) == 0) {
                 return trav;
              }
		  }
          trav = trav->next;
      }
      return NULL;
 }


dir_info * build_dir_node(uint64_t file_desc, int curr_child_index, v_file_node * v_node) {
      uint64_t addr =(uint64_t) (KB + kmalloc(PAGE_SIZE));
      //page_fault_handler(addr);
      dir_info * curr = (struct dir_info *)addr;
      memset((uint8_t *)curr, '\0', PAGE_SIZE);
      curr->des = file_desc;
      curr->curr_child_index = curr_child_index;
	  curr->v_node = v_node;
      curr->next = NULL;
      curr->prev = NULL;
      return curr;
 }



 bool add_dir(int curr_child_index, v_file_node * v_node) {
	if (s_cur_run_task == NULL) {
		kprintf("\nKernel Panic : No current running process");
	    return false;
	}
  
      dir_info * trav = (dir_info *)s_cur_run_task->file_root;
      dir_info * curr = build_dir_node(curr_available_file_des_num++, curr_child_index, v_node);
      curr->next = trav;
      s_cur_run_task->file_root = curr;
      if (trav != NULL) {
          trav->prev = curr;
      }
      return true;
 }

bool delete_dir(uint64_t file_des) {
     dir_info * dir_entry = find_dir(file_des);
     if (dir_entry == NULL) {
          kprintf("Dir entry doesn't exists to be deleted");
          return false;
      }
      dir_info * trav = s_cur_run_task->file_root;
  
      // if first node is to be deleted
      if (trav->des == file_des) {
          dir_info * next = trav->next;
          if (next != NULL) {
              next->prev = NULL;
          }
          s_cur_run_task->file_root = next;
          kfree((uint64_t)dir_entry);
          return true;
      }
  
      while (trav->next->des != dir_entry->des) {
          trav = trav->next;
      }
     trav->next = dir_entry->next;
     if (dir_entry->next != NULL) {
         dir_entry->next->prev = trav;
     }
     kfree((uint64_t)dir_entry);
     return true;
 }



 void print_dir() {
	struct dir_info * root = s_cur_run_task->file_root;
	if (root == NULL) {
		kprintf("\nRoot node is NULL");
	} else {
		dir_info * trav = root;
		kprintf("\nOpen File List is : \n");
		while (trav != NULL) {
			kprintf("%d ", trav->des);
			trav = trav->next;
		}
	}
 }


dir_info * sys_opendir(char *name) {
	if (name != NULL) {
		dir_info * temp = find_dir_by_name(name);
		if (temp != NULL) {
			return temp;
		}
	}	
	v_file_node* search_node = (v_file_node *)search_file(name, tarfs_mount_node);
	if (search_node == NULL) {
		kprintf("\n No folder exists with absolute path - %s", name);
		return NULL;
	}
	if (search_node->is_dir == 0) {
		kprintf("\n File but not folder exists with absolute path - %s", name);
		return NULL;
	}
	int current_child_index = search_node->no_of_child == 0 ? -1 : search_node->no_of_child - 1;
	if (add_dir(current_child_index, search_node)) {
		return find_dir(curr_available_file_des_num - 1);
	}
	return NULL;
}


int sys_closedir(dir_info *dirp) {
	if (dirp == NULL) {
		kprintf("\n Input passed is NULL");
		return -1;
	}
	dir_info * search_node = find_dir(dirp->des);
	if (search_node == NULL) {
		kprintf("\n No opendir for this request");
		return -1;
	}
	delete_dir(dirp->des);
	return 0;
}

struct dirent * sys_readdir(dir_info *dirp) {
	if (dirp == NULL) {
		kprintf("\n Input passed is NULL");
	    return NULL;
	}

    dir_info * search_node = find_dir(dirp->des);
	if (search_node == NULL) {
		kprintf("\n No opendir for this request");
		return NULL;
	}

	if (search_node->curr_child_index == -1) {
		return NULL;
	}
  
    v_file_node * vnode = search_node->v_node;
	v_file_node * child = vnode->v_child[search_node->curr_child_index];
	search_node->curr_child_index -= 1;	
	uint64_t addr = KB + kmalloc(sizeof(dirent));
	dirent * curr = (dirent *)addr;
	memset((uint8_t *)curr, '\0', PAGE_SIZE);
	memcpy(curr->d_name, child->v_name, kstrlen(child->v_name));
	kprintf("\n%s", curr->d_name);   // printing name.
	return curr;
}

int sys_open(const char *pathname, int flags) {
	if (pathname != NULL) {
         dir_info * temp = find_dir_by_name(pathname);
         if (temp != NULL) {
             return temp->des;
         }
     }
     v_file_node* search_node = search_file(pathname, tarfs_mount_node);
     if (search_node == NULL) {
         kprintf("\n No file exists with absolute path - %s", pathname);
         return -1;
     }
     if (search_node->is_dir == 1) {
         kprintf("\n Folder but not file exists with absolute path - %s", pathname);
         return -1;
     }
     int current_child_index = search_node->no_of_child == 0 ? -1 : search_node->no_of_child - 1;
     if (add_dir(current_child_index, search_node)) {
         return curr_available_file_des_num - 1;
     }
	return -1;
}


int sys_close(int fd) {
	if (fd < 3) {
         kprintf("\n Invalid file descriptor");
         return -1;
     }
     
	 dir_info* search_node = find_dir(fd);
     if (search_node == NULL) {
         kprintf("\n No open file for this request");
         return -1;
     }
	if (search_node->v_node->is_dir == 1) {
		kprintf("\n Cannot close directory, call closedir()");
		return -1;
	}	
     delete_dir(fd);
     return 0;
}

int sys_read(int fd, void *buf, int count) {
	if (fd < 3) {
		kprintf("\n Invalid file descriptor");
		return -1;
	 }
	dir_info* search_node = find_dir(fd);
	if (search_node == NULL) {
		kprintf("\n No open file for this request");
		return -1;
	}
	if (search_node->v_node->is_dir == 1) {
		kprintf("\n Cannnot read directory, call readdir()");
	}   
	uint64_t start_addr = search_node->v_node->start_addr;
	uint64_t end_addr = search_node->v_node->end_addr;
	uint64_t min = end_addr - start_addr;
	if (min > count) {
		min = count;
	}
	memset(buf, '\0', count);
	memcpy(buf, (void *)start_addr, min);
	return count;
}


