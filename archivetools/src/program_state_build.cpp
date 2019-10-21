//
//  program_state_build.cpp
//  playbuilder
//
//  Created by Gwynne Raskind on 12/17/15.
//  Copyright © 2015 Playnet, Inc. All rights reserved.
//

#include "informational.hpp"
#include "program_state.hpp"
extern "C" {
#include "parchive.h"
}

#include <sys/time.h>
#if __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

static inline timespec monotonic_clock(void)
{
#if __APPLE__
    static clock_serv_t clock_service = MACH_PORT_NULL;
    mach_timespec_t mts;
    
    if (clock_service == MACH_PORT_NULL) {
        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &clock_service);
    }
    clock_get_time(clock_service, &mts);
    return { mts.tv_sec, mts.tv_nsec };
#else
    timespec ts;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
#endif
}

struct Progress
{
    printf_function(3,4) void ninfo(bool debug, printf_format_arg(0) const char * const format, ...)
    {
        if (debug && verbose) {
            fprintf(stderr, "\n");
        }
        va_list args;
        va_start(args, format);
        vinfo(debug, format, args);
        va_end(args);
    }
    double elapsed_to_now(timespec &ts)
    {
        timespec now = monotonic_clock();
        return static_cast<double>(now.tv_sec - ts.tv_sec) + ((now.tv_nsec - ts.tv_nsec) / static_cast<double>(NSEC_PER_SEC));
    }
    string format_interval(double interval)
    {
        if (isnan(interval) || isinf(interval)) {
            return "??:??";
        }
        
        // could use OS routines, but this isn't worth all that work
        if (interval >= 3600) {
            return bprintf("%02.0f:%02.0f:%02.0f", floor(interval / 3600.0), floor(fmod(interval, 3600.0) / 60.0), floor(fmod(interval, 60.0)));
        } else {
            return bprintf("%02.0f:%02.0f", floor(interval / 60.0), floor(fmod(interval, 60.0)));
        }
    }
    string format_rate(double bps)
    {
        if (isnan(bps) || isinf(bps)) {
            return "??? b/s";
        }
        if (bps > 1073741824.0) {
            return bprintf("%7.3f G/s", bps / 1073741824.0);
        } else if (bps > 1048576.0) {
            return bprintf("%7.3f M/s", bps / 1048576.0);
        } else if (bps > 1024.0) {
            return bprintf("%7.3f K/s", bps / 1024.0);
        } else {
            return bprintf("%7.3f b/s", bps);
        }
    }
    
    static constexpr const size_t WINDOW = 4096;
    static_assert(__builtin_popcount(WINDOW) == 1, "power of two");

    timespec overall_start_time = { 0, 0 }, archive_start_time = { 0, 0 }, file_start_time = { 0, 0 };
    size_t currentArchiveNumber = 0, currentArchiveNumFiles = 0, currentArchiveTotalBytes = 0, currentArchiveExpectedFiles = 0;
    size_t overallTotalBytes = 0, overallNumFiles = 0, overallExpectedFiles = 0;
    size_t asidx = 0, osidx = 0;
    double asamples[WINDOW] = {0.0}, osamples[WINDOW] = {0.0};
    bool verbose, debug;
    
    Progress(bool v, bool d) : verbose(v), debug(d) {}
    double aavg() { double t = 0; for (size_t i = 0; i < WINDOW; ++i) { t += asamples[i]; } return t / std::min<double>(WINDOW, overallNumFiles); }
    double oavg() { double t = 0; for (size_t i = 0; i < WINDOW; ++i) { t += osamples[i]; } return t / std::min<double>(WINDOW, overallNumFiles); }
    void samp(double s) { asamples[asidx++ & (WINDOW - 1)] = s; osamples[osidx++ & (WINDOW - 1)] = s; }
    
    void progress_bar()
    {
        if (!verbose) {
            return;
        }
        double overall_completion = (overallNumFiles / static_cast<double>(overallExpectedFiles));
        double archive_completion = (currentArchiveNumFiles / static_cast<double>(currentArchiveExpectedFiles));
        double archive_elapsed = elapsed_to_now(archive_start_time),
               archive_guessed_eta = (archive_elapsed / archive_completion) * (1.0 - archive_completion);
        double overall_elapsed = elapsed_to_now(overall_start_time),
               overall_guessed_eta = (overall_elapsed / overall_completion) * (1.0 - overall_completion);
        const size_t numOverallStars = 10, numArchiveStars = 10;
        
        if (currentArchiveExpectedFiles == 0) { archive_completion = 0.0; archive_guessed_eta = 0.0; }
        if (overallNumFiles == 0) { overall_completion = 0.0; overall_guessed_eta = 0.0; }

        size_t currentOverallStars = (numOverallStars * overall_completion), currentArchiveStars = (numArchiveStars * archive_completion);

        fprintf(stderr, "\ra:%02zu %5zu/%5zu %s %s/%s [%s%s]  %5zu/%5zu %s %s/%s [%s%s]            ",
            currentArchiveNumber, currentArchiveNumFiles, currentArchiveExpectedFiles,
            format_rate(aavg()).c_str(), format_interval(archive_elapsed).c_str(), format_interval(archive_guessed_eta).c_str(),
            string(currentArchiveStars, '*').c_str(), string(numArchiveStars - currentArchiveStars, ' ').c_str(),
            overallNumFiles, overallExpectedFiles, format_rate(oavg()).c_str(),
            format_interval(overall_elapsed).c_str(), format_interval(overall_guessed_eta).c_str(),
            string(currentOverallStars, '*').c_str(), string(numOverallStars - currentOverallStars, ' ').c_str()
        );
    }
    void start_processing(size_t numfiles)
    {
        overall_start_time = monotonic_clock();
        overallExpectedFiles = numfiles;
    }
    void start_archive(size_t archiveNumber, size_t numfiles)
    {
        archive_start_time = monotonic_clock();
        currentArchiveNumber = archiveNumber;
        currentArchiveNumFiles = 0;
        currentArchiveExpectedFiles = numfiles;
        currentArchiveTotalBytes = 0;
        std::fill(std::begin(asamples), std::end(asamples), 0.0);
        asidx = 0;
        note(verbose, " Starting archive %zu with %zu files", archiveNumber, numfiles);
        progress_bar();
    }
    void start_file(const string &path)
    {
        file_start_time = monotonic_clock();
        
        ninfo(debug, "  Starting file %4zu of %4zu for #%2zu: %s",
            currentArchiveNumFiles + 1, currentArchiveExpectedFiles, currentArchiveNumber, path.c_str());
        progress_bar();
    }
    void finish_file(size_t filesize)
    {
        double elapsed = elapsed_to_now(file_start_time),
               bytes_per_sec = filesize / elapsed;
        
        samp(bytes_per_sec);
        currentArchiveTotalBytes += filesize;
        overallTotalBytes += filesize;
        ++overallNumFiles;
        ++currentArchiveNumFiles;
        ninfo(debug, "  Finished copying, %zu bytes took %.3f seconds (%s)", filesize, elapsed, format_rate(bytes_per_sec).c_str());
        progress_bar();
    }
    void finish_archive()
    {
        double elapsed = elapsed_to_now(archive_start_time), bytes_per_sec = currentArchiveTotalBytes / elapsed;
        
        progress_bar();
        if (verbose) { fprintf(stderr, "\n"); }
        note(verbose, " Finished archive, %zu files (%zu bytes) took %.3f seconds (%s)",
            currentArchiveExpectedFiles, currentArchiveTotalBytes, elapsed, format_rate(bytes_per_sec).c_str());
    }
    void finish_processing()
    {
        double elapsed = elapsed_to_now(overall_start_time), bytes_per_sec = overallTotalBytes / elapsed;
        
        note(verbose, "Finished, %zu files (%zu bytes) took %.3f seconds (%s)",
            overallNumFiles, overallTotalBytes, elapsed, format_rate(bytes_per_sec).c_str());
    }
};

void program_state::make_single_archive(const program_state::archive_plan &plan, const string &archive_name, void *p_)
{
    Progress *p = reinterpret_cast<Progress *>(p_);
    string marker = "DATA" + string(1, DIRECTORY_SEPARATOR);

    unique_ptr<paFILE, decay_t<decltype(paFree)>> archive(paAlloc(), paFree);
    
    paOpen(archive.get(), bprintf("%s%c%s.ww2", output_directory.c_str(), DIRECTORY_SEPARATOR, archive_name.c_str()).c_str(), "wb+");

    for (const string &path : plan.paths) {
        string::size_type loc = path.find(marker);
        
        if (loc == string::npos) {
            NOTE("Missing path marker in %s", path.c_str());
            continue;
        }
        if (path.length() - loc < 1 || path.length() - loc > 128) {
            NOTE("Member path too short or too long: %s", path.c_str());
            continue;
        }

        size_t filesize = file_size_on_disk(path);

        if (filesize > INT32_MAX) {
            NOTE("Member larger than 2GB: %s", path.c_str());
            continue;
        }

        p->start_file(path.substr(loc + marker.length()).c_str());
        unique_ptr<uint8_t[]> data = make_unique<uint8_t[]>(filesize);
        unique_ptr<FILE, int (*)(FILE *)> item(fopen(path.c_str(), "rb"), fclose);
        
        if (!item) {
            NOTE("Failed to open item %s", path.c_str());
            continue;
        }
        if (fread(data.get(), 1, filesize, item.get()) != filesize) {
            NOTE("Failed to read item %s", path.c_str());
            continue;
        }
        paAppendmember(archive.get(), path.substr(loc + marker.length()).c_str(), data.get(), filesize);
        p->finish_file(filesize);
    }
}

void program_state::run_plan(void)
{
    Progress *p = new Progress(verbose, debug);
    
    size_t nf = 0; for (auto p : plans) { nf += p.paths.size(); }
    p->start_processing(nf);
    for (const archive_plan &plan : plans) {
//        NOTE("Building archive %02u (%zu files)...", plan.number, plan.paths.size());
        p->start_archive(plan.number, plan.paths.size());
        make_single_archive(plan, plans.size() == 1 ? "playarc" : bprintf("playarc%02u", plan.number), p);
        p->finish_archive();
//        NOTE("Completed.");
    };
    p->finish_processing();
}
