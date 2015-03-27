from ftsweb.models import ACTIVE_STATES, FILE_TERMINAL_STATES


def _are_job_and_files_consistent(job):
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


def _diagnosed(job_list):
    for j in job_list:
        j['diagnosis'] = _are_job_and_files_consistent(j)
        yield j


def JobDiagnosis(job_list, require_diagnosis, require_debug, multireplica):
    def _diagnosis_filter(job):
        if require_diagnosis and job['diagnosis'] is not None:
            return False
        if require_debug and job['with_debug'] < 1:
            return False
        if multireplica and job['n_replicas'] >= job['count']:
            return False
        return True
    # We can not diagnose all jobs, that would be way too expensive!
    limit = 50
    result = list()
    for job in _diagnosed(job_list):
        if _diagnosis_filter(job):
            result.append(job)
            limit -= 1
        if limit <= 0:
            break
    return result
