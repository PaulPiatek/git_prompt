#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <git2.h>
#include <git2/status.h>

#ifndef COLOR
#define RAW
#endif

template <typename T>
std::string to_string(T value) // missing on cygwin libstdc++ ?
{
//create an output string stream
    std::ostringstream os ;
//throw the value into the string stream
    os << value ;
//convert the string stream into a string and return
    return os.str() ;
}


int main(void)
{
    setlocale(LC_ALL, "");
    git_libgit2_init();

    // current dir
    char* dir;
    dir = getcwd(NULL, 0);
    //std::cout << dir << std::endl;

    // define variables
    git_repository *repo = NULL;
    git_status_list *status = NULL;
    git_reference *head = NULL;
    const char *branch = NULL;
    int error;

    // open git repo
    error = git_repository_open_ext(&repo, dir, 0, NULL);
    if (error != 0) // no repo
    {
        return(0);
    }

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                 GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

    // branch
    error = git_repository_head(&head, repo);
    if (error != 0) // no repo
    {
        return(0);
    }
    branch = git_reference_shorthand(head);
    //std::cout << "branch: " << branch << std::endl;

    // before/ahead
    size_t ahead, behind = (size_t)0;
    git_oid masterOid, originMasterOid;
    if (!git_reference_name_to_id(&masterOid, repo, "refs/heads/master"))
    {
        if (!git_reference_name_to_id(&originMasterOid, repo, "refs/remotes/origin/master"))
        {
            if (!git_graph_ahead_behind(&ahead, &behind, repo, &masterOid, &originMasterOid))
            {
            }
            else // no ahead_behind
            {
                ahead = 0;
                behind = 0;
            }
        }
        else // no remote
        {
                ahead = 0;
                behind = 0;            
        }
    }
    
    //std::cout << "ahead: " << ahead << std::endl;
    //std::cout << "behind: " << behind << std::endl;

    // status
    git_status_list_new(&status, repo, &opts);
    size_t count = git_status_list_entrycount(status);
    //std::cout << "listsize: " << count << std::endl;

    bool i_new = false, i_mod = false, i_del = false, i_ren = false, i_chng = false, w_new = false, w_mod = false, w_del = false, w_chng = false, w_ren = false, w_con = false, flag = false;
    for (size_t i = 0; i < count; ++i)
    {
        const git_status_entry *entry = git_status_byindex(status, i);
        switch (entry->status)
        {
        case GIT_STATUS_INDEX_NEW:
            i_new = true;
            flag = true;
            break;
        case GIT_STATUS_INDEX_MODIFIED:
            i_mod = true;
            flag = true;
            break;
        case GIT_STATUS_INDEX_DELETED:
            i_del = true;
            flag = true;
            break;
        case GIT_STATUS_INDEX_RENAMED:
            i_ren = true;
            flag = true;
            break;
        case GIT_STATUS_INDEX_TYPECHANGE:
            i_chng = true;
            flag = true;
            break;

        case GIT_STATUS_WT_NEW:
            w_new = true;
            flag = true;
            break;
        case GIT_STATUS_WT_MODIFIED:
            w_mod = true;
            flag = true;
            break;
        case GIT_STATUS_WT_DELETED:
            w_del = true;
            flag = true;
            break;
        case GIT_STATUS_WT_TYPECHANGE:
            w_chng = true;
            flag = true;
            break;
        case GIT_STATUS_WT_RENAMED:
            w_ren = true;
            flag = true;
            break;

        case GIT_STATUS_CONFLICTED:
            w_con = true;
            flag = true;
            break;

        default: //GIT_STATUS_WT_UNREADABLE, GIT_STATUS_IGNORED
            break;
        }
        /*
        std::cout << "worktree modified: " << w_mod << std::endl;
        std::cout << "worktree new: " << (entry->status == GIT_STATUS_WT_NEW) << std::endl;
        std::cout << "index new: " << (entry->status == GIT_STATUS_INDEX_NEW) << std::endl;
        std::cout << "index mod: " << (entry->status == GIT_STATUS_INDEX_MODIFIED) << std::endl;
        std::cout << "index del: " << (entry->status == GIT_STATUS_INDEX_DELETED) << std::endl;
        */
    }

    // print string
    std::string s;
    
    #ifdef COLOR
    std::string c_bracket;
    std::string c_text;
    std::string c_reset;
    char* sty = std::getenv("STY");
    if (sty != NULL && std::strcmp("VSCode", sty) == 0)
    {
        c_bracket = "\033[33m";
        c_text = "\033[32m";
        c_reset = "\033[0m";
    }
    else
    {
        c_bracket = "\033[0,100m";
        c_text = "\033[0,40m";
        c_reset = "\033[0m";
    }
    s = c_bracket + "[" + c_text + branch;
    #elif defined(RAW)
    s = branch;
    #endif
    

    if (flag)
    {
        s += " ";
        if (w_new)
            s += "?";
        if (w_mod || w_ren || w_chng)    
            s += "~";
        if (w_del)    
            s += "-";
        if (i_new || i_mod || i_ren || i_chng || i_del)
            s += "*";
        if (w_con)    
            s += "#";            
    }

    if (ahead != 0 || behind != 0)
    {
        s += " ";
        if (ahead != 0)
            s += "↑" + to_string(ahead);
        if (behind != 0)
            s += "↓" + to_string(behind);  
    }

    #ifdef COLOR
    s += c_bracket + "]" + c_reset;
    #endif
    

    std::cout << s << std::endl;
    
    git_status_list_free(status);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return (0);
}
