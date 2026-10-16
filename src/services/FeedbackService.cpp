#include "services/FeedbackService.h"
#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"
FeedbackService::FeedbackService(Repository&r,StudentService&s,EventService&e):repository_(r),students_(s),events_(e){}
void FeedbackService::submit(const std::string&studentId,const std::string&eventId,int rating,const std::string&comment){students_.get(studentId);events_.get(eventId);if(rating<1||rating>5)throw InvalidInputException("Rating must be between 1 and 5.");repository_.save(Feedback{makeId("FB"),studentId,eventId,rating,comment,nowTimestamp()});}
double FeedbackService::averageRating(const std::string&eventId)const{events_.get(eventId);double total=0;int count=0;for(const auto&f:repository_.allFeedback())if(f.eventId==eventId){total+=f.rating;++count;}return count?total/count:0.0;}
std::vector<Feedback> FeedbackService::forEvent(const std::string&eventId)const{std::vector<Feedback> result;for(const auto&f:repository_.allFeedback())if(f.eventId==eventId)result.push_back(f);return result;}
