#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

/*
 * DESIGN JUSTIFICATION (Q3 - AudioBook):
 * Base class chosen: ElectronicResource
 *
 * Justification:
 * An audio book in modern academic and digital libraries is delivered as a digital
 * stream or downloadable electronic audio media, accessed via a digital platform URL.
 * It is subject to concurrent-user licensing models (such as annual subscriptions or
 * perpetual access seats) and platform hosting fees. Therefore, inheriting from
 * ElectronicResource provides accurate semantics (isDigital() == true, accessUrl,
 * licenseModel, platformFee, seat-based tiered pricing) without code duplication.
 */

class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id, std::string title, std::string publisher,
              int year, Money pricePerSeat, std::string accessUrl,
              std::string narrator, int durationMinutes,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

    ResourceCategory category() const override { return ResourceCategory::AudioBook; }

    std::string serialize() const override;

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt
