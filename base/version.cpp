// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"


namespace base
{
    namespace
    {
        // Parses the |numbers| vector representing the different numbers
        // inside the version string and constructs a vector of valid integers. It stops
        // when it reaches an invalid item (including the wildcard character). |parsed|
        // is the resulting integer vector. Function returns true if all numbers were
        // parsed successfully, false otherwise.
        bool ParseVersionNumbers(std::string_view version_str, std::vector<uint32_t>* parsed)
        {
            auto numbers = strings::split<decltype(version_str)::value_type>(version_str, ".");
            if (numbers.empty())
                return false;

            for (auto it = numbers.begin(); it != numbers.end(); ++it) {
                if (strings::starts_with<decltype(version_str)::value_type>(*it, "+"))
                    return false;

                unsigned int num = std::stoul(std::string(*it));

                // This throws out leading zeros for the first item only.
                if (it == numbers.begin() && std::to_string(num) != *it)
                    return false;

                // StringToUint returns unsigned int but Version fields are uint32_t.
                static_assert(sizeof(uint32_t) == sizeof(unsigned int),
                    "uint32_t must be same as unsigned int");
                parsed->push_back(num);
            }
            return true;
        }

        // Compares version components in |components1| with components in
        // |components2|. Returns -1, 0 or 1 if |components1| is less than, equal to,
        // or greater than |components2|, respectively.
        int CompareVersionComponents(const std::vector<uint32_t>& components1, const std::vector<uint32_t>& components2)
        {
            const size_t count = std::min(components1.size(), components2.size());
            for (size_t i = 0; i < count; ++i) {
                if (components1[i] > components2[i])
                    return 1;
                if (components1[i] < components2[i])
                    return -1;
            }
            if (components1.size() > components2.size()) {
                for (size_t i = count; i < components1.size(); ++i) {
                    if (components1[i] > 0)
                        return 1;
                }
            }
            else if (components1.size() < components2.size()) {
                for (size_t i = count; i < components2.size(); ++i) {
                    if (components2[i] > 0)
                        return -1;
                }
            }
            return 0;
        }

    }  // namespace

    Version::Version(std::string_view version_str)
    {
        std::vector<uint32_t> parsed;
        if (!ParseVersionNumbers(version_str, &parsed))
            return;

        _Components.swap(parsed);
    }

    Version::Version(std::vector<uint32_t> components)
        : _Components(std::move(components))
    {}

    bool Version::IsValid() const {
        return (!_Components.empty());
    }

    // static
    bool Version::IsValidWildcardString(_In_ std::string_view wildcard_string)
    {
        std::string_view version_string = wildcard_string;
        if (strings::ends_with<decltype(version_string)::value_type>(version_string, ".*"))
            version_string = version_string.substr(0, version_string.size() - 2);

        Version version(version_string);
        return version.IsValid();
    }

    int Version::CompareToWildcardString(_In_ std::string_view wildcard_string) const
    {
        // Default behavior if the string doesn't end with a wildcard.
        if (!strings::ends_with<decltype(wildcard_string)::value_type>(wildcard_string, ".*"))
        {
            Version version(wildcard_string);
            return CompareTo(version);
        }

        std::vector<uint32_t> parsed;
        const bool success = ParseVersionNumbers(
            wildcard_string.substr(0, wildcard_string.length() - 2), &parsed);
        const int comparison = CompareVersionComponents(_Components, parsed);
        // If the version is smaller than the wildcard version's |parsed| vector,
        // then the wildcard has no effect (e.g. comparing 1.2.3 and 1.3.*) and the
        // version is still smaller. Same logic for equality (e.g. comparing 1.2.2 to
        // 1.2.2.* is 0 regardless of the wildcard). Under this logic,
        // 1.2.0.0.0.0 compared to 1.2.* is 0.
        if (comparison == -1 || comparison == 0)
            return comparison;

        // Catch the case where the digits of |parsed| are found in |_Components|,
        // which means that the two are equal since |parsed| has a trailing "*".
        // (e.g. 1.2.3 vs. 1.2.* will return 0). All other cases return 1 since
        // components is greater (e.g. 3.2.3 vs 1.*).
        const size_t min_num_comp = std::min(_Components.size(), parsed.size());
        for (size_t i = 0; i < min_num_comp; ++i)
        {
            if (_Components[i] != parsed[i])
                return 1;
        }
        return 0;
    }

    int Version::CompareTo(_In_ const Version & other) const
    {
        return CompareVersionComponents(_Components, other._Components);
    }

    std::string Version::GetString() const
    {
        std::string version_str;
        size_t count = _Components.size();
        for (size_t i = 0; i < count - 1; ++i) {
            version_str.append(std::to_string(_Components[i]));
            version_str.append(".");
        }

        version_str.append(std::to_string(_Components[count - 1]));
        return version_str;
    }
}
