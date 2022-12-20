
namespace timer
{
  inline volatile unsigned int milliseconds = 0;

  static inline void process_interrupt() {
    ++milliseconds;
  }

}