#include "alpha_record.h"
#include "ensdf_types.h"
#include <boost/algorithm/string.hpp>

bool AlphaRecord::match(const std::string& line)
{
  return match_first(line, "\\sA");
}

AlphaRecord::AlphaRecord(size_t& idx,
                         const std::vector<std::string>& data)
{
  if ((idx >= data.size()) || !match(data[idx]))
    return;
  const auto& line = data[idx];

  nuclide = parse_nid(line.substr(0,5));
  energy = Energy(parse_val_uncert(line.substr(9,10),
                                   line.substr(19,2)));
  intensity_alpha = parse_norm_value(line.substr(21,8),
                                     line.substr(29,2));
  hindrance_factor = parse_norm_value(line.substr(31,8),
                                      line.substr(39,2));

  comment_flag = boost::trim_copy(line.substr(76,1));
  quality = boost::trim_copy(line.substr(79,1));

  while ((idx+1 < data.size()) &&
         CommentsRecord::match(data[idx+1], "A"))
    comments.push_back(CommentsRecord(++idx, data));
}

bool AlphaRecord::valid() const
{
  return nuclide.valid();
}

std::string AlphaRecord::debug() const
{
  std::string ret;
  ret = nuclide.symbolicName() + " ALPHA ";
  if (energy.valid())
    ret += " Energy=" + energy.to_string();
  if (intensity_alpha.hasFiniteValue())
    ret += " Intensity(A)=" + intensity_alpha.to_string(true);
  if (hindrance_factor.hasFiniteValue())
    ret += " HindranceFactor=" + hindrance_factor.to_string(true);
  if (!comment_flag.empty())
    ret += " comment=" + comment_flag;
  if (!quality.empty())
    ret += " quality=" + quality;
  for (auto c : comments)
    ret += "\n  Comment: " + c.debug();
  return ret;
}