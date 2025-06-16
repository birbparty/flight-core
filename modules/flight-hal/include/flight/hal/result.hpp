#ifndef FLIGHT_HAL_RESULT_HPP
#define FLIGHT_HAL_RESULT_HPP

#include <cstdint>
#include <type_traits>
#include <utility>
#include <new>

namespace flight
{
    namespace hal
    {

        // Error codes for HAL operations
        enum class ErrorCode : uint32_t
        {
            None = 0,

            // General errors
            Unknown = 1,
            InvalidArgument = 2,
            OutOfMemory = 3,
            NotImplemented = 4,

            // File errors
            FileNotFound = 100,
            PermissionDenied = 101,
            FileAlreadyExists = 102,
            NotAFile = 103,
            IsDirectory = 104,
            TooManyOpenFiles = 105,
            DiskFull = 106,
            IoError = 107,

            // Thread errors
            ThreadCreationFailed = 200,
            DeadlockDetected = 201,
            NotSupported = 202,

            // Memory errors
            MappingFailed = 300,
            InvalidAddress = 301,
            AccessViolation = 302,

            // System errors
            SystemCallFailed = 400,
            Timeout = 401,
            Interrupted = 402
        };

        // Convert error code to string for debugging
        inline const char *error_code_to_string(ErrorCode code) noexcept
        {
            switch (code)
            {
            case ErrorCode::None:
                return "None";
            case ErrorCode::Unknown:
                return "Unknown";
            case ErrorCode::InvalidArgument:
                return "InvalidArgument";
            case ErrorCode::OutOfMemory:
                return "OutOfMemory";
            case ErrorCode::NotImplemented:
                return "NotImplemented";
            case ErrorCode::FileNotFound:
                return "FileNotFound";
            case ErrorCode::PermissionDenied:
                return "PermissionDenied";
            case ErrorCode::FileAlreadyExists:
                return "FileAlreadyExists";
            case ErrorCode::NotAFile:
                return "NotAFile";
            case ErrorCode::IsDirectory:
                return "IsDirectory";
            case ErrorCode::TooManyOpenFiles:
                return "TooManyOpenFiles";
            case ErrorCode::DiskFull:
                return "DiskFull";
            case ErrorCode::IoError:
                return "IoError";
            case ErrorCode::ThreadCreationFailed:
                return "ThreadCreationFailed";
            case ErrorCode::DeadlockDetected:
                return "DeadlockDetected";
            case ErrorCode::NotSupported:
                return "NotSupported";
            case ErrorCode::MappingFailed:
                return "MappingFailed";
            case ErrorCode::InvalidAddress:
                return "InvalidAddress";
            case ErrorCode::AccessViolation:
                return "AccessViolation";
            case ErrorCode::SystemCallFailed:
                return "SystemCallFailed";
            case ErrorCode::Timeout:
                return "Timeout";
            case ErrorCode::Interrupted:
                return "Interrupted";
            default:
                return "UnknownError";
            }
        }

        // Forward declaration
        template <typename T, typename E = ErrorCode>
        class Result;

        // Helper to detect if type is a Result
        template <typename T>
        struct is_result : std::false_type
        {
        };

        template <typename T, typename E>
        struct is_result<Result<T, E>> : std::true_type
        {
        };

        // Result type for error handling without exceptions
        template <typename T, typename E>
        class Result
        {
        private:
            union Storage
            {
                typename std::aligned_storage<sizeof(T), alignof(T)>::type value_storage;
                E error;

                Storage() {}
                ~Storage() {}
            } storage_;

            bool has_value_;

            void destroy_value()
            {
                if (has_value_)
                {
                    reinterpret_cast<T *>(&storage_.value_storage)->~T();
                }
            }

            void construct_value(const T &val)
            {
                new (&storage_.value_storage) T(val);
                has_value_ = true;
            }

            void construct_value(T &&val)
            {
                new (&storage_.value_storage) T(std::move(val));
                has_value_ = true;
            }

            template <typename... Args>
            void emplace_value(Args &&...args)
            {
                new (&storage_.value_storage) T(std::forward<Args>(args)...);
                has_value_ = true;
            }

        public:
            // Constructors
            Result(const T &value) : has_value_(false)
            {
                construct_value(value);
            }

            Result(T &&value) : has_value_(false)
            {
                construct_value(std::move(value));
            }

            Result(E error) : has_value_(false)
            {
                storage_.error = error;
            }

            // Copy constructor
            Result(const Result &other) : has_value_(false)
            {
                if (other.has_value_)
                {
                    construct_value(*other);
                }
                else
                {
                    storage_.error = other.storage_.error;
                }
            }

            // Move constructor
            Result(Result &&other) noexcept : has_value_(false)
            {
                if (other.has_value_)
                {
                    construct_value(std::move(*other));
                }
                else
                {
                    storage_.error = other.storage_.error;
                }
            }

            // Destructor
            ~Result()
            {
                destroy_value();
            }

            // Assignment operators
            Result &operator=(const Result &other)
            {
                if (this != &other)
                {
                    destroy_value();
                    has_value_ = false;

                    if (other.has_value_)
                    {
                        construct_value(*other);
                    }
                    else
                    {
                        storage_.error = other.storage_.error;
                    }
                }
                return *this;
            }

            Result &operator=(Result &&other) noexcept
            {
                if (this != &other)
                {
                    destroy_value();
                    has_value_ = false;

                    if (other.has_value_)
                    {
                        construct_value(std::move(*other));
                    }
                    else
                    {
                        storage_.error = other.storage_.error;
                    }
                }
                return *this;
            }

            // Status checks
            bool is_ok() const noexcept { return has_value_; }
            bool is_err() const noexcept { return !has_value_; }
            explicit operator bool() const noexcept { return has_value_; }

            // Value access
            T &value() &
            {
                return *reinterpret_cast<T *>(&storage_.value_storage);
            }

            const T &value() const &
            {
                return *reinterpret_cast<const T *>(&storage_.value_storage);
            }

            T &&value() &&
            {
                return std::move(*reinterpret_cast<T *>(&storage_.value_storage));
            }

            T &operator*() & { return value(); }
            const T &operator*() const & { return value(); }
            T &&operator*() && { return std::move(value()); }

            T *operator->() { return &value(); }
            const T *operator->() const { return &value(); }

            // Error access
            E &error() & { return storage_.error; }
            const E &error() const & { return storage_.error; }

            // Monadic operations
            template <typename F>
            auto map(F &&func) -> Result<decltype(func(std::declval<T>())), E>
            {
                using RetType = decltype(func(std::declval<T>()));
                if (is_ok())
                {
                    return Result<RetType, E>(func(value()));
                }
                return Result<RetType, E>(error());
            }

            template <typename F>
            auto and_then(F &&func) -> decltype(func(std::declval<T>()))
            {
                using RetType = decltype(func(std::declval<T>()));
                static_assert(is_result<RetType>::value, "and_then function must return a Result type");

                if (is_ok())
                {
                    return func(value());
                }
                return RetType(error());
            }

            // Value or default
            T value_or(T &&default_value) const &
            {
                return is_ok() ? value() : std::move(default_value);
            }

            T value_or(T &&default_value) &&
            {
                return is_ok() ? std::move(value()) : std::move(default_value);
            }
        };

        // Specialization for void
        template <typename E>
        class Result<void, E>
        {
        private:
            E error_;
            bool has_value_;

        public:
            Result() : has_value_(true) {}
            Result(E error) : error_(error), has_value_(false) {}

            bool is_ok() const noexcept { return has_value_; }
            bool is_err() const noexcept { return !has_value_; }
            explicit operator bool() const noexcept { return has_value_; }

            E &error() & { return error_; }
            const E &error() const & { return error_; }

            template <typename F>
            auto and_then(F &&func) -> decltype(func())
            {
                if (is_ok())
                {
                    return func();
                }
                return decltype(func())(error_);
            }
        };

        // Helper functions
        template <typename T>
        Result<T, ErrorCode> make_ok(T &&value)
        {
            return Result<T, ErrorCode>(std::forward<T>(value));
        }

        template <typename T>
        Result<T, ErrorCode> make_error(ErrorCode error)
        {
            return Result<T, ErrorCode>(error);
        }

        inline Result<void, ErrorCode> make_ok()
        {
            return Result<void, ErrorCode>();
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_RESULT_HPP
