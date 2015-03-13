from ftsweb.models import ACTIVE_STATES, FILE_TERMINAL_STATES


def are_job_and_files_consistent(job):
    """
    Returns None if the job state and individual file states are consistent, otherwise a
    diagnosis message
    i.e.
        job in terminal, all files in terminal
        job non terminal, at least one file not terminal
    """
    non_terminal_count = sum(
        map(lambda (state, count): count if state not in FILE_TERMINAL_STATES else 0, job['files'].iteritems())
    )

    if job['job_state'] in ACTIVE_STATES and non_terminal_count == 0:
        return 'The job is in a non-terminal state, but all its transfers are in terminal states'
    if job['job_state'] not in ACTIVE_STATES and non_terminal_count > 0:
        return 'The job is in a terminal state, but has %d files in a non-terminal state' % non_terminal_count
    if job['job_state'] not in ACTIVE_STATES and not job['job_finished']:
        return 'The job is in a terminal state, but job_finished is not set'

    return None


def diagnosed(job_list):
    for j in job_list:
        j['diagnosis'] = are_job_and_files_consistent(j)
        yield j


def JobDiagnosis(job_list, require_diagnosis, require_debug):
    def _diagnosis_filter(job):
        if require_diagnosis and job['diagnosis'] is not None:
            return False
        if require_debug and job['with_debug'] < 1:
            return False
        return True
    return filter(_diagnosis_filter, diagnosed(job_list))
