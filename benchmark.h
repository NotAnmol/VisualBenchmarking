#pragma once

#define ON 1
#define OFF 0

#define BENCHMARKING ON

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>

struct ProfileResult {
    std::string Name;
    long long Start, End;
    ProfileResult(const char* name, long long start, long long end) 
        : Name(name), Start(start), End(end) {}
};

struct InstrumentationSession {
    std::string Name;
};

class Instrumentor {
    InstrumentationSession* m_CurrentSession;
    std::ofstream m_OutputStream;
    int m_ProfileCount;
public:
    Instrumentor()
        : m_CurrentSession(nullptr), m_ProfileCount(0) {}

    void BeginSession(const std::string& name, const std::string& filepath = "results.json") {
        m_OutputStream.open(filepath);
        WriteHeader();
        m_CurrentSession = new InstrumentationSession;
        m_CurrentSession->Name = name;
    }

    void EndSession() {
        WriteFooter();
        m_OutputStream.close();
        delete m_CurrentSession;
        m_CurrentSession = nullptr;
        m_ProfileCount = 0;
    }

    void WriteProfile(const ProfileResult& result) {
        if (m_ProfileCount++ > 0)
            m_OutputStream << ",";

        std::string name = result.Name;
        std::replace(name.begin(), name.end(), '"', '\'');

        m_OutputStream << "{";
        m_OutputStream << "\"cat\":\"function\",";
        m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
        m_OutputStream << "\"name\":\"" << name << "\",";
        m_OutputStream << "\"ph\":\"X\",";
        m_OutputStream << "\"pid\":0,";
        m_OutputStream << "\"tid\":0,";
        m_OutputStream << "\"ts\":" << result.Start;
        m_OutputStream << "}";

        m_OutputStream.flush();
    }

    void WriteHeader() {
        m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
        m_OutputStream.flush();
    }

    void WriteFooter() {
        m_OutputStream << "]}";
        m_OutputStream.flush();
    }

    static Instrumentor& Get() {
        static Instrumentor* instance = new Instrumentor();
        return *instance;
    }
};

class InstrumentationTimer {
    const char* m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
public:
    InstrumentationTimer(const char* name)
        : m_Name(name), m_Stopped(false) {

        m_StartTimepoint = std::chrono::high_resolution_clock::now();
    }

    ~InstrumentationTimer() {
        if (!m_Stopped)
            Stop();
    }

    void Stop() {
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
        
        Instrumentor::Get().WriteProfile(ProfileResult(m_Name, start, end));

        m_Stopped = true;
    }
};

#if BENCHMARKING
    #define PROFILE_SCOPE(name) InstrumentationTimer timer(name)
    #define START_SESSION(name) Instrumentor::Get().BeginSession(name)
    #define END_SESSION() Instrumentor::Get().EndSession()
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
    #define PROFILE_FUNCTION_DETAILED() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
    #define PROFILE_FUNCTION() 
    #define PROFILE_FUNCTION_DETAILED()
#endif