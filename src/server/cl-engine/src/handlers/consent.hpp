#ifndef CONSENT_HPP
#define CONSENT_HPP

/*
This handler only checks if the client is allowed to continue
*/

inline void consent_to_proceed_request(const char *const str) {
    int tid;
    char path[1024];
    sscanf(str, "%d %d %s", &tid, path);

}


#endif //CONSENT_HPP
