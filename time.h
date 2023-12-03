enum TimeUnit {
  MINUTES, 
  HOURS,
  DAYS, 
  MONTHS,
  YEARS, 
  SECONDS=7
};

struct Time {
  int minute;
  int hour;
  int day;
  int month;
  int year;
  float second; 
};