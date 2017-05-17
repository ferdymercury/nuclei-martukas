#include "qvalue_record.h"
#include "ensdf_types.h"
#include <sstream>
#include "custom_logger.h"
#include <boost/algorithm/string.hpp>

bool QValueRecord::match(const std::string& line)
{
  return match_first(line, "\\sQ");
}

QValueRecord::QValueRecord(size_t& idx,
                           const std::vector<std::string>& data,
                           bool recurse)
{
  if ((idx >= data.size()) || !match(data[idx]))
    return;
  const auto& line = data[idx];

  nuclide = parse_check_nid(line.substr(0, 5));

  Q = parse_val_uncert(line.substr(8, 9), line.substr(19, 2));
  SN = parse_val_uncert(line.substr(21, 7), line.substr(29, 2));
  SP = parse_val_uncert(line.substr(31, 7), line.substr(39, 2));
  QA = parse_val_uncert(line.substr(41, 7), line.substr(49, 5));

  ref = boost::trim_copy(line.substr(55, 24));

  if (!recurse)
      return;

  bool altcomment {false};
  while ((idx+1 < data.size()) &&
         (match_cont(data[idx+1], "\\sQ") ||
          CommentsRecord::match(data[idx+1], "Q")))
  {
    ++idx;
    if (CommentsRecord::match(data[idx]))
    {
      auto cr = CommentsRecord(idx, data);
      if (boost::contains(cr.text, "Current evaluation has used the following Q record"))
      {
        if (altcomment)
          DBG << "<QValueRecord::parse> 2nd altcomment for " << debug();
        altcomment = true;
      }
      else
        comments.push_back(cr);
    }
    else
    {
      if (!altcomment)
        DBG << "<QValueRecord::parse> No altcomment for " << debug();
      auto alt = QValueRecord(idx, data, false);
      if (!alternative)
        alternative =  std::shared_ptr<QValueRecord>
            (new QValueRecord(alt));
      else
        DBG << "<QValueRecord::parse> 2nd alt for " << debug();
    }
  }
}

bool QValueRecord::valid() const
{
  return nuclide.valid();
}

std::string QValueRecord::debug() const
{
  auto ret = nuclide.symbolicName() + " QVALUE "
      + " Q=" + Q.to_string(true)
      + " SN=" + SN.to_string(true)
      + " SP=" + SP.to_string(true)
      + " QA=" + QA.to_string(true)
      + " ref=" + ref;
  if (alternative)
    ret += "\n      EvalQ=" + alternative->debug();
  for (auto c : comments)
    ret += "\n      " + c.debug();
  return ret;
}
