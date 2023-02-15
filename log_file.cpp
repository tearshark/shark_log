#include "log_file.h"

namespace shark_log
{
    struct log_stdfile : log_file
    {
        FILE* file;

        log_stdfile(FILE* f) : file(f) {}

        virtual ~log_stdfile()
        {
            if (file != nullptr)
            {
                fclose(file);
            }
        }

        virtual bool write(const void* buff, size_t length) override
        {
            if (file == nullptr) return false;
            return fwrite(buff, 1, length, file) == length;
        }

        virtual bool read(void* buff, size_t length) override
        {
            if (file == nullptr) return false;
            return fread(buff, 1, length, file) == length;
        }
    };

    struct log_stdfile_factory : log_file_factory
    {
        virtual log_file* create(const std::string& path, bool writeMode) override
        {
            FILE* file;
            if (fopen_s(&file, path.c_str(), writeMode ? "wb" : "rb") != 0)
                return nullptr;
            return new log_stdfile(file);
        }
    };

    log_file_factory* shark_log_stdfile_factory()
    {
        static log_stdfile_factory factory;
        return &factory;
    }
}
