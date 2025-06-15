#include <cstdio>
#include <vector>
#include <hs/hs.h>

// int search(std::vector<const char*> patterns, const char *inputData, unsigned int length, match_event_handler eventHandler) {
int search(const char* pattern, const char *inputData, size_t length, match_event_handler eventHandler) {

    /* First, we attempt to compile the pattern provided on the command line.
     * We assume 'DOTALL' semantics, meaning that the '.' meta-character will
     * match newline characters. The compiler will analyse the given pattern and
     * either return a compiled Hyperscan database, or an error message
     * explaining why the pattern didn't compile.
     */
    hs_database_t *database;
    hs_compile_error_t *compile_err;

    // err = hs_compile_multi(expressions.data(), flags.data(), ids.data(),
    //                        expressions.size(), mode, nullptr, &db, &compileErr);

    auto flags = HS_FLAG_DOTALL | HS_FLAG_CASELESS | HS_FLAG_MULTILINE | HS_FLAG_SOM_LEFTMOST;
    if (hs_compile(pattern, flags, HS_MODE_STREAM | HS_MODE_SOM_HORIZON_LARGE, NULL, &database,
                   &compile_err) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to compile pattern \"%s\": %s\n",
                pattern, compile_err->message);
        hs_free_compile_error(compile_err);
        return -1;
    }

    if (!inputData) {
        hs_free_database(database);
        return -1;
    }

    /* Finally, we issue a call to hs_scan, which will search the input buffer
     * for the pattern represented in the bytecode. Note that in order to do
     * this, scratch space needs to be allocated with the hs_alloc_scratch
     * function. In typical usage, you would reuse this scratch space for many
     * calls to hs_scan, but as we're only doing one, we'll be allocating it
     * and deallocating it as soon as our matching is done.
     *
     * When matches occur, the specified callback function (eventHandler in
     * this file) will be called. Note that although it is reminiscent of
     * asynchronous APIs, Hyperscan operates synchronously: all matches will be
     * found, and all callbacks issued, *before* hs_scan returns.
     *
     * In this example, we provide the input pattern as the context pointer so
     * that the callback is able to print out the pattern that matched on each
     * match event.
     */
    hs_scratch_t *scratch = NULL;
    if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
        hs_free_database(database);
        return -1;
    }

    hs_stream_t *stream;

    if (hs_open_stream(database, 0, &stream) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to open stream. Exiting.\n");
        hs_free_scratch(scratch);
        hs_free_database(database);
        return -1;
    }

    printf("Scanning %llu bytes with Hyperscan\n", length);

    for (size_t offset = 0; offset < length; offset += 0x80000000) {
        size_t chunk_size = std::min(length - offset, (size_t)0x80000000);

        if (hs_scan_stream(stream, inputData + offset, chunk_size, 0, scratch, eventHandler, (void*)pattern) != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
            hs_free_scratch(scratch);
            hs_free_database(database);
            return -1;
        }
    }
    /* Scanning is complete, any matches have been handled, so now we just
     * clean up and exit.
     */
    hs_free_scratch(scratch);
    hs_free_database(database);
    return 0;
}
