enum log_importance {
    log_importance_debug,
    log_importance_warning,
    log_importance_error,
};
typedef enum log_importance log_importance;

#define log_debug(...) \
    (log_(__FILE__, __LINE__, \
          log_importance_debug, __VA_ARGS__))
#define log_warning(...) \
    (log_(__FILE__, __LINE__, \
          log_importance_warning, __VA_ARGS__))
#define log_error(...) \
    (log_(__FILE__, __LINE__, \
          log_importance_error, __VA_ARGS__))

void log_(const char* filename, unsigned long line,
          log_importance importance, const char* format, ...);
void log_set_minimum_importance(log_importance importance);
