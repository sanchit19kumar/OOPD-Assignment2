#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

/*
 * DESIGN JUSTIFICATION (Q3 - Thesis):
 * Base class chosen: Resource
 *
 * Justification:
 * A thesis is an institutional academic monograph submitted to a university. In academic
 * libraries, theses are maintained as permanent archival holdings (physical print volumes
 * or institutional open-access repository items). They are usually deposited without a purchase
 * price (free of cost: Money::of(0)), do not incur commercial publisher platform hosting fees,
 * and are not licensed through concurrent-seat models like commercial electronic databases.
 * Therefore, deriving directly from Resource correctly models its nature without carrying
 * extraneous electronic licensing fields.
 */

class Thesis : public Resource {
public:
    Thesis(std::string id, std::string title, std::string university,
           std::string degree, std::string supervisor, int year,
           Money unitPrice = Money::of(0), std::string publisher = "University Archive");

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override { return ResourceCategory::Thesis; }

    std::string serialize() const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt
