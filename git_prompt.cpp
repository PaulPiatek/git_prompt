#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include <git2.h>
#include <git2/status.h>

#if !defined(COLOR) && !defined(RAW)
#define RAW
#endif

namespace {

template <typename T>
std::string to_str(const T &value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

// Ensures git_libgit2_shutdown() is always called, on every return path.
struct Libgit2Guard
{
    Libgit2Guard() { git_libgit2_init(); }
    ~Libgit2Guard() { git_libgit2_shutdown(); }
};

} // namespace

int main(void)
{
    std::setlocale(LC_ALL, "");

    Libgit2Guard libgit2_guard;

    // Current directory.
    char *dir = getcwd(NULL, 0);
    if (dir == NULL)
        return 0;

    git_repository *repo = NULL;
    git_status_list *status = NULL;
    git_reference *head = NULL;
    git_reference *upstream = NULL;
    size_t ahead = 0;
    size_t behind = 0;
    std::string branch;

    // Open git repo (searches parent directories).
    int error = git_repository_open_ext(&repo, dir, 0, NULL);
    free(dir);
    if (error != 0) // no repo
        return 0;

    // Branch.
    error = git_repository_head(&head, repo);
    if (error != 0) // unborn / detached / no HEAD
    {
        git_repository_free(repo);
        return 0;
    }

    const char *branch_name = git_reference_shorthand(head);
    branch = (branch_name != NULL) ? branch_name : "HEAD";

    // Ahead/behind relative to the branch's configured upstream.
    // Only tracked branches have an upstream; everything else stays 0.
    if (git_branch_upstream(&upstream, head) == 0)
    {
        const git_oid *local_oid = git_reference_target(head);
        const git_oid *upstream_oid = git_reference_target(upstream);
        if (local_oid != NULL && upstream_oid != NULL)
        {
            if (git_graph_ahead_behind(&ahead, &behind,
                                       repo, local_oid, upstream_oid) != 0)
            {
                ahead = 0;
                behind = 0;
            }
        }
    }

    // Status. entry->status is a bitmask, so test individual bits rather
    // than switching on the whole value (combined states would be missed).
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                 GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

    bool w_new = false, w_mod = false, w_del = false, i_any = false, w_con = false;

    if (git_status_list_new(&status, repo, &opts) == 0)
    {
        const size_t count = git_status_list_entrycount(status);
        for (size_t i = 0; i < count; ++i)
        {
            const git_status_entry *entry = git_status_byindex(status, i);
            if (entry == NULL)
                continue;

            const unsigned int st = entry->status;

            if (st & GIT_STATUS_WT_NEW)
                w_new = true;
            if (st & (GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_RENAMED | GIT_STATUS_WT_TYPECHANGE))
                w_mod = true;
            if (st & GIT_STATUS_WT_DELETED)
                w_del = true;
            if (st & (GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
                      GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_RENAMED |
                      GIT_STATUS_INDEX_TYPECHANGE))
                i_any = true;
            if (st & GIT_STATUS_CONFLICTED)
                w_con = true;
        }
    }

    const bool flag = w_new || w_mod || w_del || i_any || w_con;

    // Print string.
    std::string s;

#ifdef COLOR
    // Colour only when writing to a terminal, so piping stays clean.
    const bool use_color = isatty(fileno(stdout)) != 0;
    const char *c_bracket = "\033[1;33m";
    const char *c_text = "\033[1;32m";
    const char *c_reset = "\033[0m";

    const char *sty = std::getenv("STY");
    if (sty != NULL && std::strcmp(sty, "VSCode") == 0)
    {
        c_bracket = "\033[33m";
        c_text = "\033[32m";
    }

    if (use_color)
        s = std::string(c_bracket) + "[" + c_text + branch;
    else
        s = branch;
#else
    s = branch;
#endif

    if (flag)
    {
        s += " ";
        if (w_new)
            s += "?";
        if (w_mod)
            s += "~";
        if (w_del)
            s += "-";
        if (i_any)
            s += "*";
        if (w_con)
            s += "#";
    }

    if (ahead != 0 || behind != 0)
    {
        s += " ";
        if (ahead != 0)
            s += "\u2191" + to_str(ahead);
        if (behind != 0)
            s += "\u2193" + to_str(behind);
    }

#ifdef COLOR
    if (use_color)
        s += std::string(c_bracket) + "]" + c_reset;
#endif

    std::cout << s << std::endl;

    git_reference_free(upstream);
    git_reference_free(head);
    git_status_list_free(status);
    git_repository_free(repo);
    return 0;
}
