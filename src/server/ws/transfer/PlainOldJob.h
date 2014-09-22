/*
 * PlainOldJobBase.h
 *
 *  Created on: 5 Jun 2014
 *      Author: simonm
 */

#ifndef PlainOldJob_H_
#define PlainOldJob_H_

#include "db/generic/GenericDbIfce.h"

#include "common/JobParameterHandler.h"

#include "BlacklistInspector.h"
#include "JobSubmitter.h"

#include <vector>
#include <string>
#include <list>
#include <algorithm>

namespace fts3
{
namespace ws
{

/**
 * Base class for plain old jobs
 *
 * provides the API for extracting job elements from
 * a plain old job
 */
template<typename ELEMENT>
class PlainOldJobBase
{
    /// job types
    enum job_type
    {
        NORMAL,
        MULTIHOP,
        _1_TO_N,
        N_TO_1
    };
    /// functional object, a predicate that checks if a given SE is not equal to a source SE
    struct source_neq
    {
        source_neq(std::string const * se) : se(se) {}

        std::string const * se;

        bool operator()(ELEMENT* e) const
        {
            return *e->source != *se;
        }
    };
    /// functional object, a predicate that checks if a given SE is not equal to a destination SE
    struct destination_neq
    {
        destination_neq(std::string const * se) : se(se) {}

        std::string const * se;

        bool operator()(ELEMENT* e) const
        {
            return *e->dest != *se;
        }
    };

public:
    /**
     * Constructor
     *
     * @param elements : a vector containing job elements that need to be extracted
     * @param initialState : initial state for a single transfer file
     */
    PlainOldJobBase(std::vector<ELEMENT*> const & elements, std::string const & initialState) :
        elements(elements), fileIndex(0), type(get_type(elements)), initialState(initialState), srm_source(true) {}

    /// Destructor
    virtual ~PlainOldJobBase() {};

    /**
     * Gets the extracted job elements
     *
     * @param jobs : list cpntainer for the extracted job elements
     * @param inspector : BlacklistInspector instance
     */
    void get(std::list<job_element_tupple> & jobs, std::string vo)
    {
        BlacklistInspector inspector(vo);

        typename std::vector<ELEMENT*>::const_iterator it;
        for (it = elements.begin(); it != elements.end(); ++it)
            {
                job_element_tupple tupple = create_job_element(it, inspector);
                jobs.push_back(tupple);
            }
        // do blacklist inspection
        inspector.inspect();
        inspector.setWaitTimeout(jobs);
    }

    /// gets srm_source flag
    bool isSrm() const
    {
        return srm_source;
    }
    /// gets source SE
    std::string getSourceSe() const
    {
        return sourceSe;
    }
    /// gets destination SE
    std::string getDestinationSe() const
    {
        return destinationSe;
    }

protected:
    /**
     * Creates a single job element
     *
     * @param it : iterator from elements collection
     * @param inspector : BlacklistInspector instance
     */
    template <typename ITER>
    job_element_tupple create_job_element(ITER const & it, BlacklistInspector& inspector);

    /**
     * the input vector
     */
    std::vector<ELEMENT*> const & elements;

private:

    /**
     * Extracts element from iterator to pointer to ELEMENT
     *
     * @param it : iterator to pointer to ELEMENT
     * @return : reference to an ELEMENT
     */
    template <typename ITER>
    ELEMENT& element(ITER const & it) const
    {
        // extract the element from the iterator
        return *(*it);
    }

    /**
     * @param vector containing all the elements
     * @return type of the job
     */
    job_type get_type(std::vector<ELEMENT*> const & elements) const;

    /// file index
    int fileIndex;
    /// job type
    job_type type;
    /// initial state for a transfer file
    std::string const & initialState;

    /// source SE name
    std::string sourceSe;
    /// destination SE name
    std::string destinationSe;
    /// srm_source flag
    bool srm_source;
};

template<typename ELEMENT>
class PlainOldJob : public PlainOldJobBase<ELEMENT>
{

public:
    PlainOldJob(std::vector<ELEMENT*> const & elements, std::string const & initialState) :
        PlainOldJobBase<ELEMENT>(elements, initialState) {}
};

template<>
class PlainOldJob<tns3__TransferJobElement2> : public PlainOldJobBase<tns3__TransferJobElement2>
{

public:
    PlainOldJob(std::vector<tns3__TransferJobElement2*> const & elements, std::string const & initialState) :
        PlainOldJobBase<tns3__TransferJobElement2>(elements, initialState) {}

    void get(std::list<job_element_tupple> & jobs, std::string vo, JobParameterHandler & params)
    {
        BlacklistInspector inspector(vo);

        std::vector<tns3__TransferJobElement2*>::const_iterator it;
        for (it = elements.begin(); it != elements.end(); ++it)
            {
                job_element_tupple tupple = create_job_element(it, inspector);

                if((*it)->checksum)
                    {
                        tupple.checksum = *(*it)->checksum;
                        if (!params.isParamSet(JobParameterHandler::CHECKSUM_METHOD))
                            params.set(JobParameterHandler::CHECKSUM_METHOD, "relaxed");
                    }

                jobs.push_back(tupple);
            }
        // do blacklist inspection
        inspector.inspect();
        inspector.setWaitTimeout(jobs);
    }
};

template<typename ELEMENT>
typename PlainOldJobBase<ELEMENT>::job_type PlainOldJobBase<ELEMENT>::get_type(std::vector<ELEMENT*> const & elements) const
{
    // if there is just one element in the job it must be a normal job
    if (elements.size() < 2) return NORMAL;

    // check if it is 1 to N transfer
    source_neq src_neq(element(elements.begin()).source);
    if (std::find_if(elements.begin(), elements.end(), src_neq) == elements.end()) return _1_TO_N;

    // check if it is N to 1 transfer
    destination_neq dst_neq(element(elements.begin()).dest);
    if (std::find_if(elements.begin(), elements.end(), dst_neq) == elements.end()) return N_TO_1;

    // check if it is a multihop transfer
    typename std::vector<ELEMENT*>::const_iterator it1 = elements.begin(), it2 = it1 + 1;
    for (; it2 != elements.end(); ++it1, ++it2)
        {
            // if it is not multihop the only thing it can be is normal transfer job
            if (*element(it1).dest != *element(it2).source) return NORMAL;
        }

    // if we reached this point it is multihop
    return MULTIHOP;

}

template <typename ELEMENT>
template <typename ITER>
job_element_tupple PlainOldJobBase<ELEMENT>::create_job_element(ITER const & it, BlacklistInspector& inspector)
{
    // source and destination
    std::string src = *element(it).source, dest = *element(it).dest;
    // source and destination SEs
    std::string sourceSe = JobSubmitter::fileUrlToSeName(src, true);
    std::string destinationSe = JobSubmitter::fileUrlToSeName(dest);
    // make blacklist inspection
    inspector.add(sourceSe);
    inspector.add(destinationSe);
    // set the source SE for the transfer job,
    // in case of this submission type multiple
    // source/destination submission is not possible
    // so we can just pick the first one
    if (this->sourceSe.empty() && type == NORMAL)
        {
            this->sourceSe = sourceSe;
        }
    // check if all the sources use SRM protocol
    srm_source &= sourceSe.find("srm") == 0;

    // set the destination SE for the transfer job,
    // in case of this submission type multiple
    // source/destination submission is not possible
    // so we can just pick the first one
    if (this->destinationSe.empty() && type == NORMAL)
        {
            this->destinationSe = destinationSe;
        }

    job_element_tupple job_element;
    job_element.source = src;
    job_element.destination = dest;
    job_element.source_se = sourceSe;
    job_element.dest_se = destinationSe;
    job_element.filesize = 0;

    job_element.state = initialState;
    job_element.fileIndex = fileIndex;
    job_element.activity = "default";

    // if it is 1 to N transfer each one will have different file index
    if (type != N_TO_1) ++fileIndex;

    return job_element;
}

} /* namespace ws */
} /* namespace fts3 */

#endif /* PlainOldJob_H_ */
